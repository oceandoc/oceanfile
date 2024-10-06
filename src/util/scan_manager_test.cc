/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include "gtest/gtest.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

TEST(ScanManager, ParallelScan) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string path = "/usr/local/test_src";

  proto::ScanStatus scan_status;
  scan_status.set_path(path);
  ScanContext ctx;
  ctx.src = path;
  ctx.status = &scan_status;
  ctx.hash_method = common::HashMethod::Hash_NONE;
  ctx.sync_method = common::SyncMethod::Sync_SYNC;
  ctx.disable_scan_cache = false;
  EXPECT_EQ(ScanManager::Instance()->ParallelScan(&ctx),
            proto::ErrCode::Success);
}

}  // namespace util
}  // namespace oceandoc
