/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_manager.h"

#include <vector>

#include "gtest/gtest.h"
#include "src/common/sqlite_row.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

TEST(SqliteManager, ExecuteNonQuery) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  EXPECT_EQ(SqliteManager::Instance()->Init(home_dir), true);

  std::string user = "admin";
  // int affect_rows = 0;
  std::string err_msg;
  std::string sql = "SELECT salt, password FROM users WHERE user = ?;";
  std::function<void(sqlite3_stmt * stmt)> bind_callback =
      [&user](sqlite3_stmt* stmt) {
        sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);
      };
  std::vector<common::UsersRow> rows;
  auto ret = util::SqliteManager::Instance()->Select(sql, &err_msg,
                                                     bind_callback, &rows);
  if (ret) {
    LOG(ERROR) << "Select user error";
    return;
  }

  if (rows.size() > 0) {
    LOG(INFO) << util::Util::ToHexStr(rows.front().salt);
    LOG(INFO) << util::Util::ToHexStr(rows.front().password);
  }
  return;
}

}  // namespace util
}  // namespace oceandoc
