/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sync_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(SyncManager, SyncLocal) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string src("/zfs");
  std::string dst("/data");
  SyncManager::Instance()->SyncLocal(src, dst);
}

}  // namespace util
}  // namespace oceandoc
