/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/client/grpc_client/grpc_repo_client.h"

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "src/client/grpc_client/grpc_user_client.h"
#include "src/util/config_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

TEST(RepoClient, ListUserRepo) {
  std::string home_dir = util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  util::ConfigManager::Instance()->Init(home_dir +
                                        "/conf/client_base_config.json");
  auto addr = util::ConfigManager::Instance()->ServerAddr();
  auto port = std::to_string(util::ConfigManager::Instance()->GrpcServerPort());
  std::string user = "dev";
  std::string password = "just_for_test";
  std::string token;

  UserClient user_client(addr, port);
  if (user_client.Login("dev", password, &token)) {
    LOG(INFO) << "Login dev success";
  }

  RepoClient repo_client(addr, port);
  google::protobuf::Map<std::string, proto::RepoMeta> repos;
  repo_client.ListUserRepo(user, token, &repos);

  proto::RepoMeta repo;
  if (repos.empty()) {
    proto::Dir dir;
    repo_client.ListServerDir(user, token, "", &dir);
    repo_client.ListServerDir(user, token, "/tmp", &dir);
    repo_client.CreateServerDir(user, token, "/tmp/test_repo");
    repo_client.CreateRepo(user, token, "test_repo", "/tmp/test_repo", &repo);
    repo_client.ListUserRepo(user, token, &repos);
    if (!repos.empty()) {
      repo_client.DeleteRepo(user, token, repos.begin()->second.repo_uuid());
      repo_client.ListUserRepo(user, token, &repos);
      repo_client.CreateRepo(user, token, "test_repo", "/tmp/test_repo", &repo);
      repo_client.ListUserRepo(user, token, &repos);
    }
  }
}

}  // namespace client
}  // namespace oceandoc
