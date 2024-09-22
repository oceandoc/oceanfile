/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/thread_pool.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(ThreadPool, Lambda) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
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
int TestFunc(const bool* const stop) {
  absl::Mutex mtx;
  int cnt = 0;
  while (!*stop) {
    absl::MutexLock lock(&mtx);
    mtx.AwaitWithTimeout(absl::Condition(stop), absl::Seconds(2));
    LOG(INFO) << "cnt: " << cnt;
    ++cnt;
  }
  LOG(INFO) << "TestFunc Exists";
  return cnt;
}

TEST(ThreadPool, Ref) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  bool stop = false;
  auto callable = std::bind(&TestFunc, &stop);
  std::packaged_task<int()> task(callable);
  std::future<int> result = task.get_future();
  ThreadPool::Instance()->Post(task);
  Util::Sleep(8 * 1000);
  stop = true;
  auto cnt = result.get();
  LOG(INFO) << cnt;

  ThreadPool::Instance()->Stop();
}

}  // namespace util
}  // namespace oceandoc
