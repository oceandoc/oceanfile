/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/sync_manager.h"

#include "gtest/gtest.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace impl {

// TEST(SyncManager, SyncLocal) {
// ConfigManager::Instance()->Init("./conf/base_config.json");
// ThreadPool::Instance()->Init();
// std::string src("/zfs");
// std::string dst("/data");
// Util::Mkdir(dst);
// SyncContext sync_ctx;
// sync_ctx.src = src;
// sync_ctx.dst = dst;
// sync_ctx.hash_method = common::HashMethod::Hash_NONE;
// sync_ctx.sync_method = common::SyncMethod::Sync_SYNC;
// sync_ctx.skip_scan = true;
//// SyncManager::Instance()->SyncLocal(&sync_ctx);
//}

// TEST(SyncManager, SyncLocalRecursive) {
// ConfigManager::Instance()->Init("./conf/base_config.json");
// ThreadPool::Instance()->Init();
// std::string src("/usr/local/test_src");
// std::string dst("/usr/local/test_dst");
// Util::Remove(dst);
// Util::Mkdir(dst);

// SyncContext sync_ctx(4);
// sync_ctx.src = src;
// sync_ctx.dst = dst;
// sync_ctx.hash_method = common::HashMethod::Hash_NONE;
// sync_ctx.sync_method = common::SyncMethod::Sync_SYNC;
// sync_ctx.skip_scan = true;
// SyncManager::Instance()->SyncLocalRecursive(&sync_ctx);
//}

TEST(SyncManager, SyncRemote) {
  util::ConfigManager::Instance()->Init("./conf/base_config.json");
  util::ThreadPool::Instance()->Init();
  std::string src("/usr/local/test_src");
  std::string dst("/tmp/test_dst");
  common::SyncContext sync_ctx(4);
  sync_ctx.src = src;
  sync_ctx.dst = dst;
  sync_ctx.hash_method = common::HashMethod::Hash_NONE;
  sync_ctx.sync_method = common::SyncMethod::Sync_SYNC;
  sync_ctx.skip_scan = false;
  sync_ctx.repo_type = proto::RepoType::RT_Remote;
  sync_ctx.partition_size = common::NET_BUFFER_SIZE_BYTES;
  sync_ctx.remote_addr = "127.0.0.1";
  sync_ctx.remote_port = "10001";

  SyncManager::Instance()->SyncRemote(&sync_ctx);
}

}  // namespace impl
}  // namespace oceandoc
