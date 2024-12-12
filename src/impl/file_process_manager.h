/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "folly/Singleton.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class FileProcessManager final {
 private:
  friend class folly::Singleton<FileProcessManager>;
  FileProcessManager() {}

 public:
  static std::shared_ptr<FileProcessManager> Instance();

  ~FileProcessManager() {}

  bool Init() {
    queue_.reserve(10000);
    process_task_ = std::thread(std::bind(&FileProcessManager::Process, this));
    return true;
  }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
    while (!process_task_exits) {
      util::Util::Sleep(500);
    }
  }

  void Put(const proto::FileReq& req) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = queue_.find(req.request_id());
    if (it != queue_.end()) {
      it->second.partitions.insert(req.partition_num());
    } else {
      common::ReceiveContext ctx;
      if (req.repo_type() == proto::RepoType::RT_Ocean) {
        ctx.repo_uuid = req.repo_uuid();
        ctx.file_name = req.src();
      } else if (req.repo_type() == proto::RepoType::RT_Remote) {
      } else {
        LOG(ERROR) << "Unsupport repo type: " << req.repo_type();
      }

      ctx.repo_type = req.repo_type();
      ctx.dst = req.dst();  // repo dir
      ctx.file_hash = req.file_hash();
      ctx.partitions.insert(req.partition_num());
      ctx.part_num =
          util::Util::FilePartitionNum(req.file_size(), req.partition_size());
      LOG(INFO) << "Queue size: " << queue_.size();
      queue_.emplace(req.request_id(), ctx);
    }
    cv_.notify_all();
  }

  void DoProcess(const std::string& request_id,
                 const common::ReceiveContext& ctx) {
    if (ctx.part_num == ctx.partitions.size()) {
      to_delete_ctx_.push_back(request_id);
    }
  }

  void Process() {
    LOG(INFO) << "Process task running";
    while (true) {
      if (stop_.load()) {
        break;
      }

      absl::base_internal::SpinLockHolder locker(&lock_);
      if (queue_.empty()) {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait_for(lock, std::chrono::seconds(60));
      }

      for (auto it = queue_.begin(); it != queue_.end();) {
        DoProcess(it->first, it->second);
      }
    }

    for (auto it = queue_.begin(); it != queue_.end();) {
      DoProcess(it->first, it->second);
    }

    process_task_exits = true;
    LOG(INFO) << "Clean task exists";
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::unordered_map<std::string, common::ReceiveContext> queue_;
  std::vector<std::string> to_delete_ctx_;
  std::atomic<bool> stop_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
  bool process_task_exits = false;
  std::thread process_task_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H
