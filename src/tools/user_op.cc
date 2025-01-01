/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/impl/user_manager.h"
#include "src/util/config_manager.h"

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv, false);
  google::SetStderrLogging(google::GLOG_INFO);

  if (argc != 2) {
    LOG(ERROR) << "must have path arg";
    return -1;
  }

  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  oceandoc::util::ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/server_base_config.json");
  oceandoc::impl::UserManager::Instance()->Init();

  std::string user = argv[1];
  std::string token;

  std::string err_msg;
  std::vector<oceandoc::util::UsersRow> rows;
  auto ret = oceandoc::impl::UserManager::Instance()->SelectUser(user, &rows,
                                                                 &err_msg);
  if (ret) {
    return Err_Fail;
  }

  if (rows.size() > 0) {
    LOG(INFO) << rows.front().user;
    LOG(INFO) << rows.front().salt;
    LOG(INFO) << rows.front().password;
    LOG(INFO) << rows.front().create_time;
    LOG(INFO) << rows.front().update_time;
  }

  LOG(INFO) << "cannot find user: " << user;

  return 0;
}
