/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "glog/logging.h"
#include "grpcpp/alarm.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/common/blocking_queue.h"
#include "src/common/defs.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/thread_pool.h"
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
  std::vector<int32_t> mark;
};

class GCEntry {
 public:
  GCEntry(SendContext* p) : p(p) {}

  ~GCEntry() {
    if (p) {
      delete p;
      p = nullptr;
    }
  }

 private:
  SendContext* p;
};

class FileClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit FileClient(
      const std::string& addr, const std::string& port,
      const proto::RepoType repo_type,
      const int64_t partition_size = common::NET_BUFFER_SIZE_BYTES,
      const common::HashMethod hash_method = common::HashMethod::Hash_NONE)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(oceandoc::proto::OceanFile::NewStub(channel_)),
        repo_type_(repo_type),
        partition_size_(partition_size),
        hash_method_(hash_method) {
    req_.mutable_content()->resize(partition_size_);
    buffer_.resize(partition_size_);
    stub_->async()->FileOp(&context_, this);
    AddHold();
    StartRead(&res_);
    StartCall();
  }

  bool Init() {
    std::thread send_thread(&FileClient::Send, this);
    send_thread.detach();
    // auto task = std::bind(&FileClient::Send, this);
    // util::ThreadPool::Instance()->Post(task);
    auto print_task = std::bind(&FileClient::Print, this);
    util::ThreadPool::Instance()->Post(print_task);
    return true;
  }

  void OnWriteDone(bool ok) override {
    if (!ok) {
      LOG(ERROR) << "Write error";
    } else {
      LOG(ERROR) << "Write success";
    }
    write_done_ = true;
    write_cv_.notify_all();
    send_num_.fetch_add(1);
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      if (res_.op() == proto::FileOp::FileExists) {
        LOG(INFO) << res_.src() << " file type: " << res_.file_type();
        if (res_.file_type() == proto::FileType::Dir ||
            res_.file_type() == proto::FileType::Symlink) {
          if (res_.err_code() != proto::ErrCode::Success) {
            LOG(INFO) << "Store " << res_.dst() << " error";
          } else {
            LOG(INFO) << "Store " << res_.dst() << " success";
          }
        } else if (res_.file_type() == proto::FileType::Regular) {
          if (!res_.can_skip_upload()) {
            SendContext* ctx = new SendContext();
            ctx->src = res_.src();
            ctx->dst = res_.dst();
            ctx->type = res_.file_type();
            ctx->op = proto::FileOp::FilePut;
            Put(ctx);
          } else {
            LOG(INFO) << "Store " << res_.dst() << " success";
          }
        }
      } else if (res_.op() == proto::FileOp::FilePut) {
        if (res_.file_type() == proto::FileType::Regular) {
          auto it = send_ctx_map_.find(res_.uuid());
          if (it != send_ctx_map_.end()) {
            auto& mark = it->second->mark;
            if (res_.err_code() != proto::ErrCode::Success) {
              ++mark[res_.partition_num()];
            } else {
              mark[res_.partition_num()] = -1;
            }
            LOG(ERROR) << "Store: " << res_.dst()
                       << ", mark: " << mark[res_.partition_num()]
                       << ", error code: " << res_.err_code();
          } else {
            LOG(ERROR) << "Store: " << res_.dst()
                       << ", error code: " << res_.err_code();
          }
        }
      }
    } else {
      LOG(ERROR) << "Read error";
    }
    StartRead(&res_);
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

  common::SendStatus GetStatus(SendContext* ctx) {
    bool success = true;
    absl::base_internal::SpinLockHolder locker(&lock_);
    for (auto m : ctx->mark) {
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
    send_status_ = common::SendStatus::SUCCESS;
  }

  void Put(SendContext* ctx) { send_queue_.PushBack(ctx); }
  size_t Size() { return send_queue_.Size(); }
  void Stop() { stop_.store(true); }

  bool FillRequest(SendContext* ctx) {
    Reset();

    if (ctx->src.empty()) {
      LOG(ERROR) << "Empty src, this should never happen";
      return false;
    }

    req_.set_uuid(util::Util::UUID());
    req_.set_src(ctx->src);
    if (repo_type_ == proto::RepoType::RT_Ocean) {
      if (ctx->repo_uuid.empty()) {
        LOG(ERROR) << "Empty repo_uuid, this should never happen";
        return false;
      }
      if (ctx->type == proto::FileType::Dir) {
        LOG(ERROR) << "Cannot upload a dir to Ocean type repo";
        return false;
      }

      req_.set_repo_uuid(ctx->repo_uuid);
    } else {
      if (ctx->dst.empty()) {
        LOG(ERROR) << "Empty dst, this should never happen";
        return false;
      }
      req_.set_dst(ctx->dst);
    }
    req_.set_repo_type(repo_type_);
    req_.set_op(ctx->op);
    req_.set_file_type(ctx->type);

    if (ctx->type == proto::FileType::Symlink) {
      req_.set_content(ctx->content);
      LOG(INFO) << ctx->src << ", dst: " << req_.dst()
                << ", target: " << req_.content();
    }

    if (ctx->op == proto::FileOp::FilePut &&
        ctx->type != proto::FileType::Regular) {
      LOG(ERROR) << "Op and Type mismatch";
      return false;
    }

    if (ctx->type == proto::FileType::Regular ||
        ctx->type == proto::FileType::Symlink) {
      common::FileAttr attr;
      if (!util::Util::PrepareFile(ctx->src, hash_method_, partition_size_,
                                   &attr)) {
        LOG(ERROR) << "Prepare error: " << ctx->src;
        return false;
      }

      if (hash_method_ != common::HashMethod::Hash_NONE) {
        req_.set_hash(attr.hash);
      }

      req_.set_size(attr.size);
      req_.set_partition_size(partition_size_);
      req_.set_update_time(attr.update_time);
      ctx->mark.resize(attr.partition_num, 0);
    }
    return true;
  }

  void Send() {
    send_queue_.Await();
    while (true) {
      if (stop_.load()) {
        LOG(INFO) << "Interrupted";
        break;
      }

      SendContext* ctx = nullptr;
      if (!send_queue_.PopBack(&ctx)) {
        if (exists_req_num_.load() <= 0) {
          StartWritesDone();
          break;
        }
        continue;
      }

      GCEntry gc(ctx);
      if (!FillRequest(ctx)) {
        LOG(ERROR) << "Fill req error";
        continue;
      }

      if (ctx->op == proto::FileOp::FileExists) {
        write_done_.store(false);
        StartWrite(&req_);
        exists_req_num_.fetch_add(1);
        std::unique_lock<std::mutex> l(write_mu_);
        write_cv_.wait(l, [this] { return write_done_.load(); });
        continue;
      }

      send_ctx_map_.insert({req_.uuid(), ctx});
      std::ifstream file(ctx->src, std::ios::binary);
      if (!file || !file.is_open()) {
        LOG(ERROR) << "Check file exists or file permissions: " << ctx->src;
        return;
      }

      int32_t partition_num = 0;
      auto BatchSend = [this, &file](const int32_t partition_num) {
        req_.mutable_content()->resize(file.gcount());
        req_.set_partition_num(partition_num);

        std::copy(buffer_.data(), buffer_.data() + file.gcount(),
                  req_.mutable_content()->begin());
        write_done_.store(false);
        StartWrite(&req_);
        std::unique_lock<std::mutex> l(write_mu_);
        write_cv_.wait(l, [this] { return write_done_.load(); });
      };

      while (file.read(buffer_.data(), partition_size_) || file.gcount()) {
        LOG(INFO) << "Now send " << ctx->src << ", part: " << partition_num;
        BatchSend(partition_num);
        ++partition_num;
      }

      while (GetStatus(ctx) == common::SendStatus::RETRING) {
        for (size_t i = 0; i < ctx->mark.size(); ++i) {
          if (ctx->mark[i] == 1) {
            file.seekg(i * common::NET_BUFFER_SIZE_BYTES);
            if (file.read(buffer_.data(), common::NET_BUFFER_SIZE_BYTES) ||
                file.gcount()) {
              BatchSend(i);
            }
          }
        }
        util::Util::Sleep(100);
      }

      send_ctx_map_.erase(req_.uuid());
    }
    LOG(INFO) << "Send exists";
  }

  void Print() {
    while (!stop_.load()) {
      LOG(INFO) << "Queue size: " << Size() << ", send num: " << send_num_;
      util::Util::Sleep(1000);
    }
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;
  const proto::RepoType repo_type_;
  const int64_t partition_size_;
  const common::HashMethod hash_method_;
  bool done_ = false;
  grpc::ClientContext context_;
  std::vector<char> buffer_;

 private:
  proto::FileReq req_;
  proto::FileRes res_;
  std::atomic<common::SendStatus> send_status_ = common::SendStatus::SUCCESS;
  std::atomic<int32_t> exists_req_num_;

 private:
  std::atomic<bool> stop_ = false;
  std::atomic<int32_t> send_num_ = 0;
  std::atomic<bool> write_done_ = false;

  mutable absl::base_internal::SpinLock lock_;
  grpc::Status status_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::mutex write_mu_;
  std::condition_variable write_cv_;
  common::BlockingQueue<SendContext*> send_queue_;
  std::unordered_map<std::string, SendContext*> send_ctx_map_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
