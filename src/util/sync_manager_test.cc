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
  std::string src("/zfs");
  std::string dst("/data");
  Util::Mkdir(dst);
  SyncManager::Instance()->SyncLocal(src, dst, common::HashMethod::Hash_NONE,
                                     common::SyncMethod::Sync_SYNC, false,
                                     true);
}

}  // namespace util
}  // namespace oceandoc
