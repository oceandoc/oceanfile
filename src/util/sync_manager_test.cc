/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sync_manager.h"

#include "gtest/gtest.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

TEST(SyncManager, SyncLocal) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string src("/usr/local/test_src");
  std::string dst("/usr/local/test_dst");
  SyncManager::Instance()->SyncLocal(src, dst, common::HashMethod::Hash_NONE,
                                     common::SyncMethod::Sync_SYNC, false);
}

}  // namespace util
}  // namespace oceandoc
