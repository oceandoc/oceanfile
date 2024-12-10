/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/impl/user_manager.h"
#include "src/util/config_manager.h"
#include "src/util/sqlite_manager.h"

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv, false);
  google::SetStderrLogging(google::GLOG_INFO);

  if (argc != 2) {
    LOG(ERROR) << "must have path arg";
    return -1;
  }

  oceandoc::util::ConfigManager::Instance()->Init(
      "./conf/server_base_config.json");
  oceandoc::impl::UserManager::Instance()->Init();

  std::string user = argv[1];
  std::string token;

  sqlite3_stmt* stmt = nullptr;
  oceandoc::util::SqliteManager::Instance()->PrepareStatement(
      "SELECT salt, password FROM users WHERE user = ?;", &stmt);
  if (!stmt) {
    return Err_Fail;
  }

  sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string hashed_password;
    hashed_password.reserve(oceandoc::util::Util::kDerivedKeySize);
    hashed_password.append(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
        oceandoc::util::Util::kDerivedKeySize);
    LOG(INFO) << hashed_password;
  }

  LOG(INFO) << "cannot find user: " << user;

  sqlite3_finalize(stmt);
  return 0;
}
