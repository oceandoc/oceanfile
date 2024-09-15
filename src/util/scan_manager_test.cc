/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(ScanManager, Scan) {
  std::string path = "/usr/local/llvm";
  ScanManager::Instance()->Scan(path);
  ScanManager::Instance()->Print(path);
}

}  // namespace util
}  // namespace oceandoc
