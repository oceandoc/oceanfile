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
  SyncContext ctx;
  ctx.src = src;
  ctx.dst = dst;
  ctx.hash_method = common::HashMethod::Hash_NONE;
  ctx.sync_method = common::SyncMethod::Sync_SYNC;
  ctx.skip_scan = true;
  SyncManager::Instance()->SyncLocal(&ctx);
}

}  // namespace util
}  // namespace oceandoc
