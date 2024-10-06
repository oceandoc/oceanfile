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
  Util::Remove(dst);
  Util::Mkdir(dst);
  SyncManager::Instance()->SyncLocal(src, dst, common::HashMethod::Hash_NONE,
                                     common::SyncMethod::Sync_SYNC, false);

  proto::ScanStatus scan_status;
  scan_status.set_path(dst);
  ScanContext ctx;
  ctx.src = dst;
  ctx.status = &scan_status;
  ctx.hash_method = common::HashMethod::Hash_NONE;
  ctx.sync_method = common::SyncMethod::Sync_SYNC;
  ctx.disable_scan_cache = false;
  auto ret = ScanManager::Instance()->ParallelScan(&ctx);
  if (ret != proto::ErrCode::Success) {
    LOG(ERROR) << "Scan " << src << " error";
  }
}

}  // namespace util
}  // namespace oceandoc
