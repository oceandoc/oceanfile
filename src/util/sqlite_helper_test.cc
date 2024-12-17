/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_helper.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {
void CreateTableFunc() {
  const std::string sql =
      "CREATE TABLE IF NOT EXISTS test ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "user TEXT UNIQUE, "
      "salt TEXT, "
      "password TEXT);";
  std::string errMsg;
  SqliteHelper::Instance()->execSql(sql, &errMsg);
}

void UpgradeTableFunc() {
  LOG(INFO) << "UpgradeTableFunc";
  const std::string sql = "DROP TABLE IF EXISTS test;";
  std::string errMsg;
  SqliteHelper::Instance()->execSql(sql, &errMsg);
}

void setVersion(sqlite3* db_, std::string version) {
  std::string sql = "PRAGMA user_version = " + version;

  char* err_msg = nullptr;
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    sqlite3_free(err_msg);
  }
}

std::string home_dir = oceandoc::util::Util::HomeDir();
std::string test_db_path = home_dir + "/data/test.db";
sqlite3* db_ = nullptr;

// version=0, just create db
TEST(SqliteHelper, initCreate) {
  if (sqlite3_open(test_db_path.c_str(), &db_) != SQLITE_OK) {
    LOG(ERROR) << "open database error";
  }
  setVersion(db_, "0");
  EXPECT_EQ(
      SqliteHelper::Instance()->init(CreateTableFunc, UpgradeTableFunc, db_),
      true);

  EXPECT_EQ(SqliteHelper::Instance()->getUserVersion(), 0);
}

// version=1, upGrade
TEST(SqliteHelper, initUpgrade) {
  if (sqlite3_open(test_db_path.c_str(), &db_) != SQLITE_OK) {
    LOG(ERROR) << "open database error";
  }

  setVersion(db_, "1");
  EXPECT_EQ(
      SqliteHelper::Instance()->init(CreateTableFunc, UpgradeTableFunc, db_),
      true);

  EXPECT_EQ(SqliteHelper::Instance()->getUserVersion(), 2);
}

}  // namespace util
}  // namespace oceandoc
