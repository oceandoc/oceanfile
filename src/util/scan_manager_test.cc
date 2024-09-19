/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include <filesystem>

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

// TODO
// 1. millions dir set performance
// 2. billions file vector performance
// 3. dump billions file name to disk performance

TEST(ScanManager, SymlinkCharacter) {
  std::filesystem::path path("/usr/lib/llvm-14/build/Debug+Asserts");
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  LOG(INFO) << Util::CreateTime(path.string());
  LOG(INFO) << Util::UpdateTime(path.string());
  LOG(INFO) << path << " size: " << Util::FileSize(path.string());

  path = "/root/src/Dr.Q/oceanfile";
  if (std::filesystem::is_directory(path)) {
    LOG(INFO) << "is directory";
  }
  LOG(INFO) << Util::CreateTime(path.string());
  LOG(INFO) << Util::UpdateTime(path.string());

  path = "/root/src/Dr.Q/oceanfile/WORKSPACE";
  if (std::filesystem::is_regular_file(path)) {
    LOG(INFO) << "is regular file";
  }
  LOG(INFO) << Util::CreateTime(path.string());
  LOG(INFO) << Util::UpdateTime(path.string());
  LOG(INFO) << path << " size: " << std::filesystem::file_size(path);
  LOG(INFO) << path << " size: " << Util::FileSize(path.string());
}

TEST(ScanManager, Scan) {
  ScanManager::Instance()->Clear();
  // std::string path = "/usr";
  std::string path = "/usr/local/llvm";
  proto::ScanStatus scan_status;
  std::unordered_set<std::string> scanned_dirs;
  scan_status.mutable_ignored_dirs()->insert(
      {ScanManager::mark_dir_name, true});
  ScanManager::Instance()->Scan(path, &scan_status, &scanned_dirs, false);
  ScanManager::Instance()->Print(scan_status);
}

TEST(ScanManager, ParallelScan) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();

  ScanManager::Instance()->Clear();
  // std::string path = "/usr";
  std::string path = "/usr/local/llvm";
  proto::ScanStatus scan_status;
  std::unordered_set<std::string> scanned_dirs;
  scan_status.mutable_ignored_dirs()->insert(
      {ScanManager::mark_dir_name, true});
  ScanManager::Instance()->ParallelScan(path, &scan_status, &scanned_dirs,
                                        false);
  ScanManager::Instance()->Print(scan_status);
}

TEST(ScanManager, ValidateCachedStatusFile) {
  ScanManager::Instance()->Clear();
  std::string path = "/usr/local/llvm";
  proto::ScanStatus scan_status;
  std::unordered_set<std::string> scanned_dirs;
  ScanManager::Instance()->ValidateCachedStatusFile(path);
}

}  // namespace util
}  // namespace oceandoc
