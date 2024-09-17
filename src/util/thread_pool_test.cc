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

}  // namespace util
}  // namespace oceandoc
