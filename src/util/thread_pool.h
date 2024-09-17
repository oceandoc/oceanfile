/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_THREAD_POOL_H
#define BAZEL_TEMPLATE_UTIL_THREAD_POOL_H

#include <functional>
#include <future>
#include <memory>

#include "boost/asio/post.hpp"
#include "boost/asio/thread_pool.hpp"
#include "src/util/config_manager.h"

namespace oceandoc {
namespace util {

class ThreadPool final {
 private:
  friend class folly::Singleton<ThreadPool>;
  ThreadPool() : terminated(false) {}

 public:
  static std::shared_ptr<ThreadPool> Instance();

  ~ThreadPool() {
    if (!terminated.load()) {
      pool_->stop();
      pool_->join();
      terminated.store(true);
    }
  }

  bool Init() {
    auto thread_num = ConfigManager::Instance()->EventThreads();
    LOG(INFO) << "thread pool size: " << thread_num;
    pool_ = std::make_shared<boost::asio::thread_pool>(thread_num);
    return true;
  }

  void Stop() {
    if (pool_) {
      pool_->stop();
      pool_->join();
      terminated.store(true);
    }
  }

  bool Post(std::packaged_task<bool()>& task) {
    boost::asio::post(*pool_.get(), std::move(task));
    return true;
  }

  bool Post(std::function<void()> task) {
    boost::asio::post(*pool_.get(), std::move(task));
    return true;
  }

  bool Post(std::packaged_task<int()>& task) {
    boost::asio::post(*pool_.get(), std::move(task));
    return true;
  }

 private:
  std::shared_ptr<boost::asio::thread_pool> pool_;
  std::atomic_bool terminated;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_THREAD_POOL_H
