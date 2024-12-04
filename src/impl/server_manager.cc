/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/server_manager.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<ServerManager> server_manager;

std::shared_ptr<ServerManager> ServerManager::Instance() {
  return server_manager.try_get();
}

}  // namespace impl
}  // namespace oceandoc
