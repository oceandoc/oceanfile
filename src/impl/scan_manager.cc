/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/scan_manager.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<ScanManager> scan_manager;

std::shared_ptr<ScanManager> ScanManager::Instance() {
  return scan_manager.try_get();
}

}  // namespace impl
}  // namespace oceandoc
