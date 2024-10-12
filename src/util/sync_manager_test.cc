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
  SyncContext sync_ctx;
  sync_ctx.src = src;
  sync_ctx.dst = dst;
  sync_ctx.hash_method = common::HashMethod::Hash_NONE;
  sync_ctx.sync_method = common::SyncMethod::Sync_SYNC;
  sync_ctx.skip_scan = true;
  // SyncManager::Instance()->SyncLocal(&sync_ctx);
}

TEST(SyncManager, SyncRemote) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string src("/usr/local/test_src");
  std::string dst("/tmp/test_dst");
  SyncContext sync_ctx(1);
  sync_ctx.src = src;
  sync_ctx.dst = dst;
  sync_ctx.hash_method = common::HashMethod::Hash_NONE;
  sync_ctx.sync_method = common::SyncMethod::Sync_SYNC;
  sync_ctx.skip_scan = false;

  sync_ctx.remote_addr = "192.168.4.100";
  sync_ctx.remote_port = "10001";

  SyncManager::Instance()->SyncRemote(&sync_ctx);
}

}  // namespace util
}  // namespace oceandoc
