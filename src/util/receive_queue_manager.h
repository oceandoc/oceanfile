/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_RECEIVE_QUEUE_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_RECEIVE_QUEUE_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "folly/Singleton.h"
#include "src/proto/service.pb.h"
#include "src/util/config_manager.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class ReceiveContext final {
 public:
  proto::FileReq req;
  int64_t update_time = 0;
  std::set<int32_t> partitions;
  int32_t part_num = 0;
};

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
    ThreadPool::Instance()->Post(clean_task);
    ThreadPool::Instance()->Post(delete_task);
    return true;
  }

  void Stop() {
    stop_.store(true);
    while (!delete_task_exits || !clean_task_exits) {
      Util::Sleep(500);
    }
  }

  void Put(const proto::FileReq& req) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = queue_.find(req.uuid());
    if (it != queue_.end()) {
      it->second.update_time = Util::CurrentTimeMillis();
    } else {
      ReceiveContext ctx;
      ctx.req = req;
      ctx.update_time = Util::CurrentTimeMillis();
      ctx.partitions.insert(req.partition_num());
      ctx.part_num = Util::FilePartitionNum(req.size(), req.partition_size());
      queue_.emplace(req.uuid(), ctx);
    }
  }

  void Delete() {
    while (true) {
      if (stop_.load()) {
        LOG(INFO) << "ReceiveQueueManager stopped";
        break;
      }
      std::unique_lock<std::mutex> lock(mu_);
      cv_.wait(lock, [this] { return stop_.load(); });
      for (size_t i = 0; i < to_remove_files_.size(); ++i) {
        Util::Remove(to_remove_files_[i]);
        LOG(INFO) << to_remove_files_[i] << " deleted";
      }
      to_remove_files_.clear();
    }
    delete_task_exits = true;
  }

  void Clean() {
    while (true) {
      if (stop_.load()) {
        LOG(INFO) << "ReceiveQueueManager stopped";
        break;
      }
      if (GetSize() < 100) {
        Util::Sleep(1000);
      }

      absl::base_internal::SpinLockHolder locker(&lock_);
      for (auto it = queue_.begin(); it != queue_.end();) {
        if (it->second.part_num == it->second.partitions.size()) {
          it = queue_.erase(it);
          continue;
        }
        auto now = Util::CurrentTimeMillis();
        if (now - it->second.update_time <
            ConfigManager::Instance()->ReceiveQueueTimeout()) {
          ++it;
          continue;
        }
        std::unique_lock<std::mutex> lock(mu_);
        to_remove_files_.emplace_back(it->second.req.dst());
      }
    }
    clean_task_exits = true;
  }

  size_t GetSize() {
    absl::base_internal::SpinLockHolder locker(&lock_);
    return queue_.size();
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::unordered_map<std::string, ReceiveContext> queue_;
  std::atomic<bool> stop_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
  std::vector<std::string> to_remove_files_;
  bool clean_task_exits = false;
  bool delete_task_exits = false;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_RECEIVE_QUEUE_MANAGER_H
