/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H

#include <memory>
#include <string>

#include "folly/Singleton.h"
#include "sqlite3.h"  // NOLINT
#include "src/common/error.h"

namespace oceandoc {
namespace util {
class SqliteManager final {
 private:
  friend class folly::Singleton<SqliteManager>;
  SqliteManager() {}

 public:
  static std::shared_ptr<SqliteManager> Instance();

  ~SqliteManager() {
    if (db_) {
      sqlite3_close(db_);
    }
  }

  bool Init() {
    std::string user_db_path = "./data/user.db";
    if (sqlite3_open(user_db_path.c_str(), &db_) != SQLITE_OK) {
      return false;
    }

    std::string error_msg;
    if (ExecuteNonQuery("CREATE TABLE IF NOT EXISTS users ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "username TEXT UNIQUE, "
                        "password TEXT);",
                        &error_msg)) {
      LOG(ERROR) << "Init database error";
      return false;
    }

    sqlite3_stmt* stmt = nullptr;
    auto ret = util::SqliteManager::Instance()->PrepareStatement(
        "INSERT OR IGNORE INTO users (username, salt, password) VALUES (?, ?);",
        stmt);
    if (ret) {
      LOG(ERROR) << "Init admin prepare error";
      return false;
    }

    sqlite3_bind_text(stmt, 1, "admin", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, "admin", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, "admin", -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      LOG(ERROR) << "Init admin execute error";
      sqlite3_finalize(stmt);
      return false;
    }
    sqlite3_finalize(stmt);
    return true;
  }

  int32_t PrepareStatement(const std::string& query, sqlite3_stmt* stmt) {
    if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) !=
        SQLITE_OK) {
      return Err_Sql_prepare_error;
    }
    return Err_Success;
  }

  int32_t ExecuteNonQuery(const std::string& query, std::string* error_msg) {
    char* errmsg = nullptr;
    if (sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errmsg) !=
        SQLITE_OK) {
      error_msg->append(errmsg);
      sqlite3_free(errmsg);
      return Err_Sql_execute_error;
    }
    return Err_Success;
  }

 private:
  sqlite3* db_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H
