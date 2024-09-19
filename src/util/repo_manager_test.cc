/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/repo_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(RepoManager, SyncLocal) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string src("/usr/local/llvm");
  std::string dst("/usr/local/test");
  RepoManager::Instance()->SyncLocal(src, dst);
}

}  // namespace util
}  // namespace oceandoc
