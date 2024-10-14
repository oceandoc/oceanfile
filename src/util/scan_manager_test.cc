/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include <sstream>

#include "gtest/gtest.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

TEST(ScanManager, GenFileName) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();
  EXPECT_EQ(ScanManager::Instance()->GenFileName("/usr/local/test_src"),
            "/usr/local/test_src/.Dr.Q.config/"
            "d210d7935537d87080626ecd439374bccd7524dbd18e0b4a379a3fce0866cc08");
}

TEST(ScanManager, BitwiseOp) {
  std::atomic<uint64_t> t = 0;
  for (int i = 0; i < 4; ++i) {
    t.fetch_or(1ULL << i);
  }

  EXPECT_EQ(t, 15);

  for (int i = 0; i < 4; ++i) {
    EXPECT_NE(t & (1ULL << i), 0);
  }
  EXPECT_EQ(t, 15);

  for (int i = 0; i < 4; ++i) {
    t.fetch_and(~(1ULL << i));
    EXPECT_EQ(t & (1ULL << i), 0);
  }
}

std::string PrintScanStatus(const proto::ScanStatus& status) {
  std::stringstream sstream;
  for (const auto& d : status.scanned_dirs()) {
    sstream << "\nDir: " << d.first;
  }
  for (const auto& d : status.scanned_dirs()) {
    for (const auto& f : d.second.files()) {
      sstream << "\nFile: " << d.first << "/" << f.first;
    }
  }
  return sstream.str();
}

TEST(ScanManager, ParallelScan) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();

  std::string path = "test_data/util_test/test_scan";

  auto runfile_dir = Util::GetEnv("TEST_SRCDIR");
  auto workspace_name = Util::GetEnv("TEST_WORKSPACE");
  std::string final_path;
  if (runfile_dir.has_value()) {
    final_path = std::string(*runfile_dir) + "/" +
                 std::string(*workspace_name) + "/" + path;
  }

  LOG(INFO) << final_path;
  EXPECT_EQ(Util::Create(final_path + "/xiedeacc.txt"), true);

  proto::ScanStatus scan_status;
  scan_status.set_path(path);
  common::ScanContext ctx;
  ctx.src = final_path;
  ctx.status = &scan_status;
  ctx.hash_method = common::HashMethod::Hash_NONE;
  ctx.sync_method = common::SyncMethod::Sync_SYNC;
  ctx.disable_scan_cache = false;

  EXPECT_EQ(ScanManager::Instance()->ParallelScan(&ctx), Err_Success);
  EXPECT_EQ(scan_status.scanned_dirs().size(), 1);
  EXPECT_EQ(scan_status.file_num(), 1);
  EXPECT_EQ(scan_status.symlink_num(), 0);

  ctx.Reset();
  scan_status.Clear();
  scan_status.set_path(path);
  ctx.src = final_path;
  EXPECT_EQ(Util::Mkdir(final_path + "/test_dir1/test_dir2/test_dir3"), true);
  EXPECT_EQ(Util::Create(final_path + "/test_dir1/test_dir2/test_dir3/txt"),
            true);
  EXPECT_EQ(ScanManager::Instance()->ParallelScan(&ctx), Err_Success);
  LOG(INFO) << PrintScanStatus(scan_status);
  EXPECT_EQ(scan_status.scanned_dirs().size(), 5);  // add .Dr.Q.config
  EXPECT_EQ(scan_status.file_num(), 3);             // add .Dr.Q.config/xxxhash
  EXPECT_EQ(scan_status.symlink_num(), 0);

  ctx.Reset();
  scan_status.Clear();
  scan_status.set_path(path);
  ctx.src = final_path;
  EXPECT_EQ(Util::Remove(final_path + "/test_dir1/test_dir2/test_dir3"), true);
  EXPECT_EQ(ScanManager::Instance()->ParallelScan(&ctx), Err_Success);
  LOG(INFO) << PrintScanStatus(scan_status);
  EXPECT_EQ(scan_status.scanned_dirs().size(), 4);  // add .Dr.Q.config
  EXPECT_EQ(scan_status.file_num(), 3);  // add .Dr.Q.config/xxxhash.tmp
  EXPECT_EQ(scan_status.symlink_num(), 0);
  EXPECT_EQ(ctx.removed_files.size(), 2);
}

}  // namespace util
}  // namespace oceandoc
