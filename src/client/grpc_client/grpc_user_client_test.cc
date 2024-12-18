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
  util::ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/client_base_config.json");

  auto addr = util::ConfigManager::Instance()->ServerAddr();
  auto port = std::to_string(util::ConfigManager::Instance()->GrpcServerPort());
  UserClient user_client(addr, port);

  std::string user = "xie";
  std::string plain_passwd = "qazwsxedc";
  std::string token;
  user_client.Login(user, plain_passwd, &token);
  LOG(INFO) << "xie login success, token: " << token;
}

}  // namespace client
}  // namespace oceandoc
