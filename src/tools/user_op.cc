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

  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  oceandoc::util::ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/server_base_config.json");
  oceandoc::impl::UserManager::Instance()->Init();

  std::string user = argv[1];
  std::string token;

  sqlite3_stmt** stmt = nullptr;
  int affect_rows = 0;
  std::string err_msg;
  std::string sql = "SELECT salt, password FROM users WHERE user = ?;";
  std::function<void(sqlite3_stmt * stmt)> bind_callback =
      [&user](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);
      };
  auto ret = oceandoc::util::SqliteManager::Instance()->Execute(
      sql, &affect_rows, &err_msg, stmt, bind_callback);
  if (ret) {
    return Err_Fail;
  }

  if (sqlite3_step(*stmt) == SQLITE_ROW) {
    std::string hashed_password;
    hashed_password.reserve(oceandoc::util::Util::kDerivedKeySize);
    hashed_password.append(
        reinterpret_cast<const char*>(sqlite3_column_text(*stmt, 1)),
        oceandoc::util::Util::kDerivedKeySize);
    LOG(INFO) << hashed_password;
  }

  LOG(INFO) << "cannot find user: " << user;

  sqlite3_finalize(*stmt);
  return 0;
}
