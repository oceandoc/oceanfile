/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/user_manager.h"

#include "gtest/gtest.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

TEST(UserManager, RegisterUser) {
  UserManager::Instance()->Init();
  std::string user = "admin";
  std::string plain_passwd = "admin";
  std::string token;
  UserManager::Instance()->RegisterUser(user, util::Util::SHA256(plain_passwd),
                                        &token);
}

TEST(UserManager, LoginUser) {
  UserManager::Instance()->Init();
  std::string user = "admin";
  std::string plain_passwd = "admin";
  std::string token;
  UserManager::Instance()->RegisterUser(user, util::Util::SHA256(plain_passwd),
                                        &token);
}

TEST(UserManager, DeleteUser) {
  UserManager::Instance()->Init();
  std::string user = "admin";
  std::string plain_passwd = "admin";
  std::string token;
  UserManager::Instance()->RegisterUser(user, util::Util::SHA256(plain_passwd),
                                        &token);
}

}  // namespace impl
}  // namespace oceandoc
