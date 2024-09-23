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

namespace oceandoc {
namespace client {

class FileClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit FileClient(const std::string& addr, const std::string& port)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(oceandoc::proto::OceanFile::NewStub(channel_)) {}

  void OnWriteDone(bool ok) override {
    if (!ok) {
      send_status_ = common::SendStatus::FATAL;
    }
    write_cv_.notify_all();
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
    } else {
      send_status_ = common::SendStatus::FATAL;
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
    bool success = true;
    absl::base_internal::SpinLockHolder locker(&lock_);
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
    send_status_ = common::SendStatus::SUCCESS;
    done_ = false;
    req_.Clear();
    mark_.clear();

    grpc::ClientContext context;
    stub_->async()->FileOp(&context, this);

    StartRead(&res_);
    StartCall();

    util::FileAttr attr;
    if (!util::Util::PrepareFile(path, &attr)) {
      LOG(ERROR) << "Prepare error: " << path;
      return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file || !file.is_open()) {
      LOG(ERROR) << "Check file exists or file permissions: " << path;
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

    auto BatchSend = [this, buffer, &file](const int32_t partition_num) {
      req_.mutable_content()->resize(file.gcount());
      req_.set_partition_num(partition_num);

      std::copy(buffer, buffer + file.gcount(),
                req_.mutable_content()->begin());
      StartWrite(&req_);
      std::unique_lock<std::mutex> l(write_mu_);
      write_cv_.wait(l);
    };

    while (file.read(buffer, util::FilePartitionSize) || file.gcount()) {
      BatchSend(partition_num);
      if (send_status_ == common::SendStatus::FATAL) {
        StartWritesDone();
        return false;
      }
      ++partition_num;
    }

    while (GetStatus() == common::SendStatus::RETRING) {
      for (size_t i = 0; i < mark_.size(); ++i) {
        if (send_status_ == common::SendStatus::FATAL) {
          StartWritesDone();
          return false;
        }
        if (mark_[i] == 1) {
          file.seekg(i * util::FilePartitionSize);
          if (file.read(buffer, util::FilePartitionSize) || file.gcount()) {
            BatchSend(i);
          }
        }
      }
      util::Util::Sleep(10);
    }

    StartWritesDone();
    return true;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;

  grpc::Status status_;

  std::atomic<common::SendStatus> send_status_ = common::SendStatus::SUCCESS;

  std::mutex mu_;
  std::condition_variable cv_;

  std::mutex write_mu_;
  std::condition_variable write_cv_;
  bool done_ = false;
  proto::FileReq req_;
  proto::FileRes res_;
  mutable absl::base_internal::SpinLock lock_;
  std::vector<int32_t> mark_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
