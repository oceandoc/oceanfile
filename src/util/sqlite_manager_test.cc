/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_manager.h"

#include <vector>

#include "gtest/gtest.h"
#include "src/util/sqlite_row.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

int32_t SelectUser() {
  std::string user = "admin";
  std::string err_msg;
  std::string sql =
      R"(SELECT user, salt, password, create_time, update_time
         FROM users
         WHERE user = ?;)";
  SqliteBinder bind_callback = [&user](sqlite3_stmt* stmt) -> bool {
    if (sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC)) {
      return false;
    }
    return true;
  };
  std::vector<UsersRow> rows;
  auto ret = util::SqliteManager::Instance()->Select(sql, &err_msg,
                                                     bind_callback, &rows);
  if (ret) {
    LOG(ERROR) << "Select user error: " << err_msg;
    return -1;
  }

  if (rows.size() > 0) {
    LOG(INFO) << rows.front().user;
    LOG(INFO) << rows.front().salt;
    LOG(INFO) << rows.front().password;
    LOG(INFO) << rows.front().create_time;
    LOG(INFO) << rows.front().update_time;
    return rows.size();
  }
  return 0;
}

TEST(SqliteManager, Select) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  auto db_path = home_dir + "/data/fstation.db";
  if (Util::Exists(db_path)) {
    if (!Util::Remove(db_path)) {
      LOG(ERROR) << "Remove " << db_path << " error";
    }
  }

  EXPECT_EQ(SqliteManager::Instance()->Init(home_dir), true);
  EXPECT_EQ(SelectUser(), 1);
  return;
}

TEST(SqliteManager, Delete) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  EXPECT_EQ(SqliteManager::Instance()->Init(home_dir), true);
  std::string user = "admin";
  std::string err_msg;
  std::string sql = R"(DELETE FROM users WHERE user = ?;)";
  SqliteBinder bind_callback = [&user](sqlite3_stmt* stmt) -> bool {
    if (sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC)) {
      return false;
    }
    return true;
  };
  std::vector<UsersRow> rows;
  int affect_rows = 0;
  auto ret = util::SqliteManager::Instance()->Delete(sql, &affect_rows,
                                                     &err_msg, bind_callback);
  if (ret) {
    LOG(ERROR) << "Delete admin error: " << err_msg;
    return;
  }
  EXPECT_EQ(SelectUser(), 0);
}

TEST(SqliteManager, Update) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  EXPECT_EQ(SqliteManager::Instance()->Init(home_dir), true);
  EXPECT_EQ(SqliteManager::Instance()->GetVersion(), 1);

  EXPECT_EQ(SqliteManager::Instance()->UpdateVersion(2), true);
  EXPECT_EQ(SqliteManager::Instance()->GetVersion(), 2);
}

}  // namespace util
}  // namespace oceandoc
