/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_PERIODIC_TASK_H
#define BAZEL_TEMPLATE_UTIL_PERIODIC_TASK_H

#include <memory>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class PeriodicTask {
 private:
  friend class folly::Singleton<PeriodicTask>;
  PeriodicTask() = default;

 public:
  static std::shared_ptr<PeriodicTask> Instance();

  void AddTask() {}
  void RemoveTask() {}

  void Run() {
    while (!terminated.load()) {
      auto now = Util::CurrentTimeMillis();
      // TODO write read lock
      for (auto& p : task_time_info_) {
        if (now <= p.second.second) {
          p.second.second += p.second.first;
          // TODO
          // post task;
        }
      }
    }
    LOG(INFO) << "Clean thread exists!";
  }

  void Start() {
    ThreadPool::Instance()->Post(std::bind(&PeriodicTask::Run, this));
  }

 private:
  std::atomic_bool terminated = false;
  std::map<std::string, std::pair<uint32_t, int64_t>> task_time_info_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_PERIODIC_TASK_H
