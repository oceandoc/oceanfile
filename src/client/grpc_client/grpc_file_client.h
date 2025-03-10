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
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
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

class FileClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit FileClient(common::SyncContext* ctx) : sync_ctx_(ctx) {
    channel_ = grpc::CreateChannel(
        sync_ctx_->remote_addr + ":" + sync_ctx_->remote_port,
        grpc::InsecureChannelCredentials());

    stub_ = proto::OceanFile::NewStub(channel_);

    req_.mutable_content()->resize(sync_ctx_->partition_size);
    buffer_.resize(sync_ctx_->partition_size);
    stub_->async()->FileOp(&context_, this);
    StartRead(&res_);
    StartCall();
  }

  ~FileClient() {
    if (task_.joinable()) {
      task_.join();
    }
    if (print_task_.joinable()) {
      print_task_.join();
    }
  }

  void Reset() {
    req_.Clear();
    res_.Clear();
    done_ = false;
    write_done_ = false;
    send_queue_.Clear();
    send_ctx_map_.clear();
    send_task_stopped_ = false;
    print_task_stopped_ = false;
    fill_queue_complete_ = false;
    onfly_req_num_ = 0;
    skipped_num = 0;
  }

  void Start() {
    task_ = std::thread(&FileClient::Send, this);
    print_task_ = std::thread(&FileClient::Print, this);
    // NOTICE: cannot use below code
    // auto task = std::bind(&FileClient::Send, this);
    // util::ThreadPool::Instance()->Post(task);
    // auto print_task = std::bind(&FileClient::Print, this);
    // util::ThreadPool::Instance()->Post(print_task);
    Reset();
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
    if (channel_->WaitForConnected(deadline)) {
      LOG(INFO) << "Connected to the server!";
    } else {
      LOG(ERROR) << "Failed to connect to the server.";
    }
  }

  void Stop() {
    done_ = true;
    context_.TryCancel();
    write_cv_.notify_all();
    while (!send_task_stopped_ || !print_task_stopped_) {
      util::Util::Sleep(100);
    }
    cv_.notify_all();
    LOG(INFO) << "FileClient stopped";
  }

  void OnWriteDone(bool ok) override {
    if (!ok) {
      LOG(ERROR) << "Write error";
      // Stop();
    } else {
      onfly_req_num_.fetch_add(1);
    }
    write_done_.store(true);
    write_cv_.notify_all();
  }

  void OnReadDone(bool ok) override {
    if (!ok) {
      LOG(ERROR) << "Read error";
      // Stop();
      return;
    }
    if (done_) {
      LOG(INFO) << "Already stopped";
      return;
    }

    if (res_.op() == proto::FileOp::FileExists) {
      if (res_.file_type() == proto::FileType::Regular) {
        if (!res_.can_skip_upload()) {
          std::shared_ptr<common::SendContext> ctx =
              std::make_shared<common::SendContext>();
          ctx->src = res_.src();
          ctx->dst = res_.dst();
          ctx->file_type = res_.file_type();
          ctx->op = proto::FileOp::FilePut;
          Put(ctx);
        } else {
          skipped_num.fetch_add(1);
        }
      }
    } else if (res_.op() == proto::FileOp::FilePut) {
      if (res_.file_type() == proto::FileType::Regular) {
        auto it = send_ctx_map_.find(res_.request_id());
        if (it != send_ctx_map_.end()) {
          auto& mark = it->second->mark;
          if (res_.err_code() != proto::ErrCode::Success) {
            ++mark[res_.partition_num()];
          } else {
            mark[res_.partition_num()] = -1;
          }
        } else {
          LOG(ERROR) << "Miss uuid, Should never happen: " << res_.src();
        }
      }
    }
    onfly_req_num_.fetch_sub(1);
    StartRead(&res_);
  }

  void OnDone(const grpc::Status& s) override {
    LOG(INFO) << "OnDone";
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    while (!send_task_stopped_ || !print_task_stopped_) {
      util::Util::Sleep(100);
    }
    cv_.notify_all();
  }

  grpc::Status Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
  }

  common::SendStatus GetStatus(std::shared_ptr<common::SendContext> ctx) {
    bool success = true;
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

  void Put(const std::shared_ptr<common::SendContext>& ctx) {
    send_queue_.PushBack(ctx);
  }

  size_t Size() { return send_queue_.Size(); }

  bool FillRequest(std::shared_ptr<common::SendContext> ctx) {
    req_.Clear();
    if (ctx->src.empty()) {
      LOG(ERROR) << "Empty src, this should never happen";
      return false;
    }

    req_.set_request_id(util::Util::UUID());
    if (sync_ctx_->repo_type == proto::RepoType::RT_Ocean) {
      if (sync_ctx_->repo_uuid.empty()) {
        LOG(ERROR) << "Empty repo_uuid, this should never happen";
        return false;
      }
      if (ctx->file_type == proto::FileType::Direcotry) {
        LOG(ERROR) << "Cannot upload a dir to Ocean type repo";
        return false;
      }

      req_.set_repo_uuid(sync_ctx_->repo_uuid);
      req_.set_user(sync_ctx_->user);
      req_.set_token(sync_ctx_->token);
      std::filesystem::path src_path(ctx->src);
      req_.set_src(src_path.stem().string());
    } else {
      if (ctx->dst.empty()) {
        LOG(ERROR) << "Empty dst, this should never happen";
        return false;
      }
    }
    req_.set_dst(ctx->dst);
    req_.set_repo_type(sync_ctx_->repo_type);
    req_.set_op(ctx->op);
    req_.set_file_type(ctx->file_type);

    if (ctx->file_type == proto::FileType::Symlink) {
      req_.set_content(ctx->content);
      LOG(INFO) << ctx->src << ", dst: " << req_.dst()
                << ", target: " << req_.content();
    }

    if (ctx->op == proto::FileOp::FilePut &&
        ctx->file_type != proto::FileType::Regular) {
      LOG(ERROR) << "Op and Type mismatch";
      return false;
    }

    if (ctx->file_type == proto::FileType::Regular ||
        ctx->file_type == proto::FileType::Symlink) {
      common::FileAttr attr;
      if (!util::Util::PrepareFile(ctx->src, sync_ctx_->hash_method,
                                   sync_ctx_->partition_size, &attr)) {
        LOG(ERROR) << "Prepare error: " << ctx->src;
        return false;
      }

      if (sync_ctx_->hash_method != common::HashMethod::Hash_NONE) {
        req_.set_file_hash(attr.file_hash);
      }

      req_.set_file_size(attr.file_size);
      req_.set_partition_size(sync_ctx_->partition_size);
      req_.set_update_time(attr.update_time);
      ctx->mark.resize(attr.partition_num, 0);
    }
    return true;
  }

  void SetFillQueueComplete() { fill_queue_complete_ = true; }

  void Send() {
    // NOTICE: cannot use below code, race condition
    // send_queue_.Await();
    while (Size() <= 0 && !fill_queue_complete_) {
      util::Util::Sleep(200);
    }

    send_task_stopped_ = false;
    while (true) {
      if (done_) {
        LOG(INFO) << "Interrupted";
        break;
      }

      std::shared_ptr<common::SendContext> ctx = nullptr;
      if (!send_queue_.PopBack(&ctx)) {
        util::Util::Sleep(100);
        if (fill_queue_complete_ && onfly_req_num_.load() <= 0 && Size() <= 0) {
          LOG(INFO) << "StartWritesDone";
          StartWritesDone();
          break;
        }
        continue;
      }

      if (!FillRequest(ctx)) {
        LOG(ERROR) << "Fill req error";
        continue;
      }

      if (ctx->op == proto::FileOp::FileExists) {
        if (ctx->file_type == proto::FileType::Symlink) {
          LOG(INFO) << "Now send symlink: " << ctx->src;
          sync_ctx_->syncd_file_success_cnt.fetch_add(1);
        }
        write_done_.store(false);
        StartWrite(&req_);
        std::unique_lock<std::mutex> l(write_mu_);
        write_cv_.wait(l, [this] { return write_done_.load() || done_; });
        continue;
      }

      send_ctx_map_.insert({req_.request_id(), ctx});
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
        write_cv_.wait(l, [this] { return write_done_.load() || done_; });
      };

      while (file.read(buffer_.data(), sync_ctx_->partition_size) ||
             file.gcount()) {
        BatchSend(partition_num);
        if (done_) {
          break;
        }
        ++partition_num;
      }

      auto send_status = common::SendStatus::SUCCESS;
      do {
        send_status = GetStatus(ctx);
        if (done_) {
          break;
        }
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
      } while (send_status == common::SendStatus::RETRING);

      if (send_status == common::SendStatus::SUCCESS) {
        LOG(INFO) << "Send file: " << ctx->src << " success";
      } else {
        LOG(ERROR) << "Send file: " << ctx->src << " error";
      }
      sync_ctx_->syncd_file_success_cnt.fetch_add(1);
      send_ctx_map_.erase(req_.request_id());
    }
    send_task_stopped_ = true;
    LOG(INFO) << "Send exists";
  }

  void Print() {
    print_task_stopped_ = false;
    while (!done_) {
      LOG(INFO) << "Queue size: " << Size()
                << ", onfly_req_num: " << onfly_req_num_
                << ", send file num: " << sync_ctx_->syncd_file_success_cnt;
      util::Util::Sleep(1000);
    }

    LOG(INFO) << "Queue size: " << Size()
              << ", onfly_req_num: " << onfly_req_num_
              << ", send file num: " << sync_ctx_->syncd_file_success_cnt;
    print_task_stopped_ = true;
  }

 private:
  common::SyncContext* sync_ctx_;

  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<proto::OceanFile::Stub> stub_;
  bool done_ = false;
  grpc::ClientContext context_;
  std::vector<char> buffer_;
  std::thread task_;
  std::thread print_task_;

 private:
  proto::FileReq req_;
  proto::FileRes res_;
  std::atomic<int32_t> onfly_req_num_ = 0;
  std::atomic<int32_t> skipped_num = 0;

 private:
  std::mutex write_mu_;
  std::condition_variable write_cv_;
  std::atomic<bool> write_done_ = false;

  grpc::Status status_;
  std::mutex mu_;
  std::condition_variable cv_;

  common::BlockingQueue<std::shared_ptr<common::SendContext>> send_queue_;
  std::unordered_map<std::string, std::shared_ptr<common::SendContext>>
      send_ctx_map_;
  bool send_task_stopped_ = false;
  bool print_task_stopped_ = false;
  bool fill_queue_complete_ = false;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_FILE_CLIENT_H
