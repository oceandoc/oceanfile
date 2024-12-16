/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/repo_manager.h"

#include "gtest/gtest.h"
#include "src/util/config_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

TEST(RepoManager, ListRepoMediaFiles) {
  std::string home_dir = "/usr/local/fstation";
  util::ConfigManager::Instance()->Init(home_dir);
  RepoManager::Instance()->Init(home_dir);
  oceandoc::util::SqliteManager::Instance()->Init(home_dir);
  proto::RepoReq req;
  proto::RepoRes res;
  req.set_op(proto::RepoOp::RepoListRepoMediaFiles);
  auto ret = impl::RepoManager::Instance()->ListRepoMediaFiles(req, &res);
  LOG(INFO) << util::Util::MessageToJson(res);
  EXPECT_EQ(ret, Err_Success);
}

}  // namespace impl
}  // namespace oceandoc
