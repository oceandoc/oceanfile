/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sync_manager.h"

namespace oceandoc {
namespace util {

static folly::Singleton<SyncManager> sync_manager;

std::shared_ptr<SyncManager> SyncManager::Instance() {
  return sync_manager.try_get();
}

}  // namespace util
}  // namespace oceandoc
