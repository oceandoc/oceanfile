/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H

#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <vector>

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/common/structs.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

using grpc::ClientContext;
using grpc::Status;

namespace oceandoc {
namespace client {

class FileClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit FileClient(proto::OceanFile::Stub* stub) {
    stub->async()->FileOp(&context_, this);
    StartRead(&res_);
    StartCall();
  }

  void Reset() { done_ = false; }

  void OnWriteDone(bool ok) override {
    if (ok) {
      write_cv_.notify_all();
    }
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      if (res_.err_code() != proto::ErrCode::SUCCESS) {
        absl::base_internal::SpinLockHolder locker(&lock_);
        ++mark_[res_.partition_num()];
      } else {
        absl::base_internal::SpinLockHolder locker(&lock_);
        mark_[res_.partition_num()] = -1;
      }
      StartRead(&res_);
    }
  }

  void OnDone(const grpc::Status& s) override {
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
  }

  grpc::Status Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
  }

  common::SendStatus GetStatus() {
    absl::base_internal::SpinLockHolder locker(&lock_);
    bool success = true;
    for (auto m : mark_) {
      if (m > 1) {
        return common::SendStatus::TOO_MANY_RETRY;
      }
      if (m == 1) {
        success = false;
      }
    }

    return success ? common::SendStatus::SUCCESS : common::SendStatus::RETRING;
  }

  bool Send(const std::string& path) {
    util::FileAttr attr;
    if (!util::Util::PrepareFile(path, &attr)) {
      LOG(ERROR) << "Prepare error";
      return 0;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file || !file.is_open()) {
      LOG(ERROR) << "Check file exists or file permissions";
      return false;
    }

    mark_.resize(attr.partition_num, 0);

    char buffer[util::FilePartitionSize];
    req_.mutable_content()->reserve(util::FilePartitionSize);
    req_.set_op(proto::Op::File_Put);
    req_.set_path(path);
    req_.set_sha256(attr.sha256);
    req_.set_size(attr.size);

    int32_t partition_num = 0;
    while (file.read(buffer, util::FilePartitionSize) || file.gcount()) {
      req_.mutable_content()->resize(file.gcount());
      req_.set_partition_num(partition_num);
      ++partition_num;

      std::copy(buffer, buffer + file.gcount(),
                req_.mutable_content()->begin());
      StartWrite(&req_);
      std::unique_lock<std::mutex> l(write_mu_);
      write_cv_.wait(l);
    }

    while (GetStatus() == common::SendStatus::RETRING) {
      for (size_t i = 0; i < mark_.size(); ++i) {
        if (mark_[i] == 1) {
          file.seekg(i * util::FilePartitionSize);
          if (file.read(buffer, util::FilePartitionSize) || file.gcount()) {
            req_.mutable_content()->resize(file.gcount());
            req_.set_partition_num(partition_num);
            ++partition_num;

            std::copy(buffer, buffer + file.gcount(),
                      req_.mutable_content()->begin());
            StartWrite(&req_);
            std::unique_lock<std::mutex> l(write_mu_);
            write_cv_.wait(l);
          }
        }
      }
    }

    StartWritesDone();
    return true;
  }

 private:
  ClientContext context_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::mutex write_mu_;
  std::condition_variable write_cv_;
  Status status_;
  bool done_ = false;
  proto::FileReq req_;
  proto::FileRes res_;
  mutable absl::base_internal::SpinLock lock_;
  std::vector<int32_t> mark_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
