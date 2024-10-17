/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/repo_manager.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<RepoManager> repo_manager;

std::shared_ptr<RepoManager> RepoManager::Instance() {
  return repo_manager.try_get();
}

}  // namespace impl
}  // namespace oceandoc
