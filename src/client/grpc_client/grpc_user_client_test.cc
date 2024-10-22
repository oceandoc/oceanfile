/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/client/grpc_client/grpc_user_client.h"

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "src/util/config_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

TEST(UserClient, All) {
  std::string home_dir = util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  util::ConfigManager::Instance()->Init(home_dir +
                                        "/conf/client_base_config.json");

  auto addr = util::ConfigManager::Instance()->ServerAddr();
  auto port = std::to_string(util::ConfigManager::Instance()->GrpcServerPort());
  UserClient user_client(addr, port);

  std::string user = "admin";
  std::string plain_passwd = "admin";
  std::string token;
  user_client.Login(user, util::Util::SHA256(plain_passwd), &token);
  user_client.ChangePassword(user, util::Util::SHA256(plain_passwd),
                             util::Util::SHA256("just_for_test"), token);

  if (user_client.Register("dev", "just_for_test", &token)) {
    LOG(INFO) << "Register dev success, password: just_for_test";
  }
}

}  // namespace client
}  // namespace oceandoc
