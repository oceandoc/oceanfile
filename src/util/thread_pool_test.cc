/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/thread_pool.h"

#include <chrono>

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

std::mutex mu;
std::condition_variable cond_var;

TEST(ThreadPool, Lambda) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  oceandoc::util::ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/server_base_config.json");
  ThreadPool::Instance()->Init();

  auto callable = std::bind([](int a) -> int { return a + 5; }, 6);
  std::packaged_task<int()> task(callable);
  std::future<int> result = task.get_future();
  ThreadPool::Instance()->Post(task);
  auto sum = result.get();
  LOG(INFO) << sum;

  ThreadPool::Instance()->Stop();
}

// must use point
int TestFunc(const bool* stop) {
  int cnt = 0;
  while (!*stop) {
    std::unique_lock<std::mutex> lock(mu);
    if (cond_var.wait_for(lock, std::chrono::seconds(2),
                          [stop] { return *stop; })) {
      break;
    }
    LOG(INFO) << "cnt: " << cnt;
    ++cnt;
  }
  LOG(INFO) << "TestFunc Exists";
  return cnt;
}

TEST(ThreadPool, Ref) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  oceandoc::util::ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/server_base_config.json");
  ThreadPool::Instance()->Init();
  bool stop = false;
  auto callable = std::bind(&TestFunc, &stop);
  std::packaged_task<int()> task(callable);
  std::future<int> result = task.get_future();
  ThreadPool::Instance()->Post(task);
  Util::Sleep(8 * 1000);
  stop = true;
  cond_var.notify_all();
  auto cnt = result.get();
  LOG(INFO) << cnt;

  ThreadPool::Instance()->Stop();
}

}  // namespace util
}  // namespace oceandoc
