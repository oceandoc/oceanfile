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
#include "src/common/defs.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

class SendContext {
 public:
  std::string repo_uuid;
  std::string src;
  std::string dst;
  proto::FileType type;
};

class FileClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit FileClient(
      const std::string& addr, const std::string& port,
      const proto::RepoType repo_type,
      const int64_t partition_size = common::NET_BUFFER_SIZE_BYTES,
      const bool calc_hash = false)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(oceandoc::proto::OceanFile::NewStub(channel_)),
        repo_type(repo_type),
        partition_size(partition_size),
        calc_hash(calc_hash) {}

  void OnWriteDone(bool ok) override {
    if (!ok) {
      LOG(ERROR) << "Write error";
      send_status_ = common::SendStatus::FATAL;
    }
    write_cv_.notify_all();
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      if (res_.err_code() != proto::ErrCode::Success) {
        absl::base_internal::SpinLockHolder locker(&lock_);
        ++mark_[res_.partition_num()];
      } else {
        absl::base_internal::SpinLockHolder locker(&lock_);
        mark_[res_.partition_num()] = -1;
      }
      StartRead(&res_);
      LOG(ERROR) << "Store success: " << res_.hash()
                 << ", part: " << res_.partition_num();
    }
  }

  void OnDone(const grpc::Status& s) override {
    LOG(INFO) << "Finshed";
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
      if (m >= 0) {
        success = false;
      }
    }
    return success ? common::SendStatus::SUCCESS : common::SendStatus::RETRING;
  }

  void Reset() {
    req_.Clear();
    res_.Clear();
    mark_.clear();
    done_ = false;
    send_status_ = common::SendStatus::SUCCESS;
  }

  bool Send(const SendContext& ctx) {
    if (ctx.src.empty()) {
      LOG(ERROR) << "Empty src";
      return false;
    }

    if (repo_type == proto::RepoType::RT_Ocean) {
      if (ctx.repo_uuid.empty()) {
        LOG(ERROR) << "Empty repo_uuid";
        return false;
      }
      req_.set_repo_type(proto::RepoType::RT_Ocean);
      req_.set_path(ctx.src);
      req_.set_repo_uuid(ctx.repo_uuid);
      if (ctx.type == proto::FileType::Dir) {
        return true;
      }
    } else {
      if (ctx.dst.empty()) {
        LOG(ERROR) << "Empty dst";
        return false;
      }
      req_.set_repo_type(proto::RepoType::RT_Remote);
      req_.set_path(ctx.dst);
    }

    Reset();
    grpc::ClientContext context;
    stub_->async()->FileOp(&context, this);

    StartRead(&res_);
    StartCall();

    req_.set_op(proto::FileOp::FilePut);

    common::FileAttr attr;

    if (!util::Util::PrepareFile(ctx.src, calc_hash, partition_size, &attr)) {
      LOG(ERROR) << "Prepare error: " << ctx.src;
      return false;
    }
    if (calc_hash) {
      req_.set_hash(attr.hash);
    }
    req_.set_size(attr.size);
    mark_.resize(attr.partition_num, 0);

    std::ifstream file(ctx.src, std::ios::binary);
    if (!file || !file.is_open()) {
      LOG(ERROR) << "Check file exists or file permissions: " << ctx.src;
      return false;
    }

    mark_.resize(attr.partition_num, 0);

    std::vector<char> buffer(partition_size);
    req_.mutable_content()->resize(partition_size);

    int32_t partition_num = 0;
    auto BatchSend = [this, &buffer, &file](const int32_t partition_num) {
      req_.mutable_content()->resize(file.gcount());
      req_.set_partition_num(partition_num);

      std::copy(buffer.data(), buffer.data() + file.gcount(),
                req_.mutable_content()->begin());
      StartWrite(&req_);
      std::unique_lock<std::mutex> l(write_mu_);
      write_cv_.wait(l);
    };

    while (file.read(buffer.data(), partition_size) || file.gcount()) {
      LOG(INFO) << "Now send " << ctx.src << ", part: " << partition_num;
      BatchSend(partition_num);
      ++partition_num;
    }

    while (GetStatus() == common::SendStatus::RETRING) {
      for (size_t i = 0; i < mark_.size(); ++i) {
        if (mark_[i] == 1) {
          file.seekg(i * common::NET_BUFFER_SIZE_BYTES);
          if (file.read(buffer.data(), common::NET_BUFFER_SIZE_BYTES) ||
              file.gcount()) {
            BatchSend(i);
          }
        }
      }
      util::Util::Sleep(100);
    }

    bool success = true;
    for (size_t i = 0; i < mark_.size(); ++i) {
      if (mark_[i] != -1) {
        // TODO(xieyz) log failed part
        success = false;
      }
    }

    StartWritesDone();
    return success;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;

 public:
  const proto::RepoType repo_type;
  const int64_t partition_size;
  const bool calc_hash;

  proto::FileReq req_;
  proto::FileRes res_;
  std::vector<int32_t> mark_;
  bool done_ = false;
  std::atomic<common::SendStatus> send_status_ = common::SendStatus::SUCCESS;

 private:
  mutable absl::base_internal::SpinLock lock_;
  grpc::Status status_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::mutex write_mu_;
  std::condition_variable write_cv_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
