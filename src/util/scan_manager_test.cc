/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include "gtest/gtest.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

TEST(ScanManager, LoadCachedScanStatus) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string path = "/usr/local/test_src";
  proto::ScanStatus scan_status;
  ScanManager::Instance()->LoadCachedScanStatus(path, &scan_status);
  // for (const auto& p : scan_status.scanned_dirs()) {
  // LOG(INFO) << p.first;
  // }
}

TEST(ScanManager, ParallelScan) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  std::string path = "/usr/local/test_src";
  proto::ScanStatus scan_status;
  scan_status.mutable_ignored_dirs()->insert({common::CONFIG_DIR, true});
  ScanManager::Instance()->ParallelScan(path, &scan_status, false, false);
  ScanManager::Instance()->Print(scan_status);
}

}  // namespace util
}  // namespace oceandoc
