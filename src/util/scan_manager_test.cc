/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include <filesystem>

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

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
  ScanManager::Instance()->Scan(path);
  ScanManager::Instance()->Print();
}

TEST(ScanManager, ParallelScan) {
  ConfigManager::Instance()->Init("./conf/base_config.json");
  ThreadPool::Instance()->Init();

  ScanManager::Instance()->Clear();
  // std::string path = "/usr";
  std::string path = "/usr/local/llvm";
  ScanManager::Instance()->ParallelScan(path);
  ScanManager::Instance()->Print();
}

// TODO
// 1. millions dir set performance
// 2. billions file vector performance
// 3. dump billions file name to disk performance

}  // namespace util
}  // namespace oceandoc
