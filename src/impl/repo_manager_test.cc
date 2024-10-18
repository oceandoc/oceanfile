/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/repo_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace impl {

TEST(RepoManager, CreateRepo) {
  std::string path("/tmp");
  std::string uuid;
  RepoManager::Instance()->CreateRepo("test", path, &uuid);
}

}  // namespace impl
}  // namespace oceandoc
