/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/session_manager.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<SessionManager> session_manager;

std::shared_ptr<SessionManager> SessionManager::Instance() {
  return session_manager.try_get();
}

}  // namespace impl
}  // namespace oceandoc
