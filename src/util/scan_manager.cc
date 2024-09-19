/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/scan_manager.h"

namespace oceandoc {
namespace util {

static folly::Singleton<ScanManager> scan_manager;

const std::string ScanManager::mark_dir_name = ".Dr.Q.config";

std::shared_ptr<ScanManager> ScanManager::Instance() {
  return scan_manager.try_get();
}

}  // namespace util
}  // namespace oceandoc
