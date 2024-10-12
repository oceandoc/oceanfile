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
#include "src/common/blocking_queue.h"
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
  std::string content;
  std::string hash;
  proto::FileOp op;
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
        calc_hash(calc_hash) {
    StartRead(&res_);
    StartCall();
  }

  void OnWriteDone(bool ok) override {
    if (!ok) {
      LOG(ERROR) << "Write error";
      send_status_ = common::SendStatus::FATAL;
    }
    write_cv_.notify_all();
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      if (res_.file_type() == proto::FileType::Regular) {
        if (res_.err_code() != proto::ErrCode::Success) {
          {
            absl::base_internal::SpinLockHolder locker(&lock_);
            ++mark_[res_.partition_num()];
          }
          LOG(ERROR) << "Store error: " << res_.path()
                     << ", mark: " << mark_[res_.partition_num()]
                     << ", error code: " << res_.err_code();
        } else {
          absl::base_internal::SpinLockHolder locker(&lock_);
          mark_[res_.partition_num()] = -1;
        }
        StartRead(&res_);
      }
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
      if (m >= 0) {
        success = false;
      }
    }
    // LOG(INFO) << "m: " << mark_[0] << (success ? " success" : " fail");
    return success ? common::SendStatus::SUCCESS : common::SendStatus::RETRING;
  }

  void Reset() {
    req_.Clear();
    res_.Clear();
    mark_.clear();
    done_ = false;
    send_status_ = common::SendStatus::SUCCESS;
  }

  void Put(const SendContext& ctx) { send_queue_.PushBack(ctx); }
  size_t Size() { return send_queue_.Size(); }
  void Stop() { stop_.store(true); }

  bool FillRequest(const SendContext& ctx) {
    Reset();

    if (ctx.src.empty()) {
      LOG(ERROR) << "Empty src, this should never happen";
      return false;
    }

    if (repo_type == proto::RepoType::RT_Ocean) {
      if (ctx.repo_uuid.empty()) {
        LOG(ERROR) << "Empty repo_uuid, this should never happen";
        return false;
      }
      if (ctx.type == proto::FileType::Dir) {
        LOG(ERROR) << "Cannot upload a dir to Ocean type repo";
        return false;
      }

      req_.set_repo_uuid(ctx.repo_uuid);
      req_.set_path(ctx.src);
    } else {
      if (ctx.dst.empty()) {
        LOG(ERROR) << "Empty dst, this should never happen";
        return false;
      }
      req_.set_path(ctx.dst);
    }
    req_.set_repo_type(repo_type);
    req_.set_op(ctx.op);
    req_.set_file_type(ctx.type);

    if (ctx.type == proto::FileType::Symlink) {
      req_.set_content(ctx.content);
      LOG(INFO) << ctx.src << ", dst: " << req_.path()
                << ", target: " << req_.content();
    }

    if (ctx.op == proto::FileOp::FilePut &&
        ctx.type != proto::FileType::Regular) {
      LOG(ERROR) << "Op and Type mismatch";
      return false;
    }

    if (ctx.type == proto::FileType::Regular) {
      common::FileAttr attr;
      if (!util::Util::PrepareFile(ctx.src, calc_hash, partition_size, &attr)) {
        LOG(ERROR) << "Prepare error: " << ctx.src;
        return false;
      }

      if (calc_hash) {
        req_.set_hash(attr.hash);
      }

      req_.set_size(attr.size);
      req_.set_partition_size(partition_size);
      mark_.resize(attr.partition_num, 0);
    }
    return true;
  }

  bool Send(const SendContext& ctx) {
    while (true) {
      if (stop_.load()) {
        LOG(INFO) << "Interrupted";
        break;
      }

      if (!FillRequest(ctx)) {
        LOG(ERROR) << "Fill req error";
        return false;
      }

      grpc::ClientContext context;
      stub_->async()->FileOp(&context, this);

      if (ctx.type == proto::FileType::Dir ||
          ctx.type == proto::FileType::Symlink) {
        StartWrite(&req_);
        std::unique_lock<std::mutex> l(write_mu_);
        write_cv_.wait(l);
        continue;
      }

      std::ifstream file(ctx.src, std::ios::binary);
      if (!file || !file.is_open()) {
        LOG(ERROR) << "Check file exists or file permissions: " << ctx.src;
        return false;
      }

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
    }

    StartWritesDone();
    return true;
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
  std::atomic<bool> stop_ = false;

  mutable absl::base_internal::SpinLock lock_;
  grpc::Status status_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::mutex write_mu_;
  std::condition_variable write_cv_;
  common::BlockingQueue<SendContext> send_queue_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
