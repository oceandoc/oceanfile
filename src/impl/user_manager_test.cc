/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/user_manager.h"

#include "gtest/gtest.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

TEST(UserManager, UserExists) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  auto db_path = home_dir + "/data/fstation.db";
  if (util::Util::Exists(db_path)) {
    if (!util::Util::Remove(db_path)) {
      LOG(ERROR) << "Remove " << db_path << " error";
    }
  }

  EXPECT_EQ(util::SqliteManager::Instance()->Init(home_dir), true);
  EXPECT_EQ(UserManager::Instance()->Init(), true);

  std::string user = "admin";
  std::string plain_passwd = "admin";
  std::string token;
  EXPECT_EQ(UserManager::Instance()->UserExists(user), Err_Success);

  EXPECT_EQ(UserManager::Instance()->UserLogin(
                user, util::Util::SHA256(plain_passwd), &token),
            Err_Success);
  EXPECT_EQ(token.empty(), false);

  plain_passwd = "admin1";
  EXPECT_EQ(UserManager::Instance()->ChangePassword(
                "admin", util::Util::SHA256("admin"),
                util::Util::SHA256(plain_passwd), &token),
            Err_Success);
  EXPECT_EQ(UserManager::Instance()->UserExists(user), Err_Success);

  plain_passwd = "admin";
  EXPECT_EQ(UserManager::Instance()->UserLogin(
                user, util::Util::SHA256(plain_passwd), &token),
            Err_User_passwd_error);
  EXPECT_EQ(token.empty(), false);

  plain_passwd = "admin1";
  EXPECT_EQ(UserManager::Instance()->UserLogin(
                user, util::Util::SHA256(plain_passwd), &token),
            Err_Success);
  EXPECT_EQ(token.empty(), false);

  user = "xiedeacc";
  plain_passwd = "admin";
  EXPECT_EQ(UserManager::Instance()->UserRegister(
                user, util::Util::SHA256(plain_passwd), &token),
            Err_Success);
  EXPECT_EQ(UserManager::Instance()->UserExists(user), Err_Success);
  EXPECT_EQ(UserManager::Instance()->UserDelete(user, user, token),
            Err_Success);
  EXPECT_EQ(UserManager::Instance()->UserExists(user), Err_User_not_exists);
}

}  // namespace impl
}  // namespace oceandoc
