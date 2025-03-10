/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_RECEIVE_QUEUE_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_RECEIVE_QUEUE_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "folly/Singleton.h"
#include "src/impl/repo_manager.h"
#include "src/util/config_manager.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class ReceiveQueueManager final {
 private:
  friend class folly::Singleton<ReceiveQueueManager>;
  ReceiveQueueManager() {}

 public:
  static std::shared_ptr<ReceiveQueueManager> Instance();

  ~ReceiveQueueManager() {}

  bool Init() {
    queue_.reserve(10000);
    auto clean_task = std::bind(&ReceiveQueueManager::Clean, this);
    auto delete_task = std::bind(&ReceiveQueueManager::Delete, this);
    util::ThreadPool::Instance()->Post(clean_task);
    util::ThreadPool::Instance()->Post(delete_task);
    return true;
  }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
    while (!delete_task_exits || !clean_task_exits) {
      util::Util::Sleep(500);
    }
  }

  void Put(const proto::FileReq& req) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = queue_.find(req.request_id());
    if (it != queue_.end()) {
      it->second.update_time = util::Util::CurrentTimeMillis();
      it->second.partitions.insert(req.partition_num());
    } else {
      common::ReceiveContext ctx;
      if (req.repo_type() == proto::RepoType::RT_Ocean) {
        ctx.repo_uuid = req.repo_uuid();
        ctx.file_name = req.src();
      } else if (req.repo_type() == proto::RepoType::RT_Remote) {
        ctx.file_update_time = req.update_time();
      } else {
        LOG(ERROR) << "Unsupport repo type: " << req.repo_type();
      }

      ctx.repo_type = req.repo_type();
      ctx.update_time = util::Util::CurrentTimeMillis();
      ctx.dst = req.dst();  // repo dir
      ctx.file_hash = req.file_hash();
      ctx.partitions.insert(req.partition_num());
      ctx.part_num =
          util::Util::FilePartitionNum(req.file_size(), req.partition_size());
      LOG(INFO) << "Queue size: " << queue_.size();
      queue_.emplace(req.request_id(), ctx);
    }
  }

  void Delete() {
    LOG(INFO) << "Delete task running";
    while (true) {
      if (stop_.load()) {
        break;
      }
      std::unique_lock<std::mutex> lock(mu_);
      cv_.wait(lock,
               [this] { return stop_.load() || !to_remove_files_.empty(); });
      for (size_t i = 0; i < to_remove_files_.size(); ++i) {
        util::Util::Remove(to_remove_files_[i]);
        LOG(INFO) << "Store " << to_remove_files_[i] << " error";
      }
      to_remove_files_.clear();
    }
    delete_task_exits = true;
    LOG(INFO) << "Delete task exists";
  }

  void Clean() {
    LOG(INFO) << "Clean task running";
    int64_t last_time = util::Util::CurrentTimeMillis();
    while (true) {
      if (stop_.load()) {
        break;
      }
      util::Util::Sleep(500);

      auto now = util::Util::CurrentTimeMillis();
      auto offset = now - last_time;
      if (GetSize() <= 5 && offset < 1000 * 5) {
        continue;
      }
      last_time = now;

      absl::base_internal::SpinLockHolder locker(&lock_);
      for (auto it = queue_.begin(); it != queue_.end();) {
        const auto ctx = it->second;
        if (it->second.part_num == it->second.partitions.size()) {
          if (it->second.repo_type == proto::RepoType::RT_Ocean) {
            if (RepoManager::Instance()->CreateRepoFile(ctx)) {
              {
                std::unique_lock<std::mutex> lock(mu_);
                to_remove_files_.emplace_back(it->second.dst);
              }
              cv_.notify_all();
              it = queue_.erase(it);
              continue;
            }
          } else if (it->second.repo_type == proto::RepoType::RT_Remote) {
            if (!util::Util::SetUpdateTime(ctx.dst, ctx.file_update_time)) {
              LOG(INFO) << "Set update time error: " << ctx.dst;
              {
                std::unique_lock<std::mutex> lock(mu_);
                to_remove_files_.emplace_back(it->second.dst);
              }
              cv_.notify_all();
              it = queue_.erase(it);
              continue;
            }
          } else {
            LOG(ERROR) << "Unsupport repo type: " << it->second.repo_type;
            continue;
          }

          LOG(INFO) << "Store " << it->second.dst
                    << (it->second.repo_type == proto::RepoType::RT_Ocean
                            ? std::string("/") + ctx.file_name
                            : "")
                    << " success";
          it = queue_.erase(it);
          continue;
        }
        if (now - it->second.update_time <
            util::ConfigManager::Instance()->ReceiveQueueTimeout()) {
          ++it;
          continue;
        }

        {
          std::unique_lock<std::mutex> lock(mu_);
          to_remove_files_.emplace_back(it->second.dst);
        }
        it = queue_.erase(it);
        cv_.notify_all();
      }
    }
    clean_task_exits = true;
    LOG(INFO) << "Clean task exists";
  }

  size_t GetSize() {
    absl::base_internal::SpinLockHolder locker(&lock_);
    return queue_.size();
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::unordered_map<std::string, common::ReceiveContext> queue_;
  std::atomic<bool> stop_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
  std::vector<std::string> to_remove_files_;
  bool clean_task_exits = false;
  bool delete_task_exits = false;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_RECEIVE_QUEUE_MANAGER_H
