/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SQLITE_HELPER_H
#define BAZEL_TEMPLATE_UTIL_SQLITE_HELPER_H

#include <functional>
#include <iostream>
#include <mutex>
#include <string>

#include "external/sqlite/sqlite3.h"
#include "folly/Singleton.h"
#include "src/common/error.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class SqliteHelper final {
 private:
  friend class folly::Singleton<SqliteHelper>;
  SqliteHelper() {}

 public:
  using CreateTableCallback = std::function<void()>;
  using UpgradeTableCallback = std::function<void()>;

  static std::shared_ptr<SqliteHelper> Instance();

  ~SqliteHelper() {
    if (db_) {
      sqlite3_close(db_);
    }
  }

  int32_t execSql(const std::string& sql, std::string* errmsg) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
      errmsg->append(err_msg);
      sqlite3_free(err_msg);
      return Err_Sql_execute_error;
    }
    return Err_Success;
  }

 public:
  sqlite3* db_;
  std::mutex mutex_;
  CreateTableCallback onCreate_;
  UpgradeTableCallback onUpgrade_;

  bool init(CreateTableCallback onCreate, UpgradeTableCallback onUpgrade,
            sqlite3* db) {
    onCreate_ = onCreate;
    onUpgrade_ = onUpgrade;
    db_ = db;

    if (db_) {
      return checkAndUpgradeDatabase();
    }
    return false;
  }

  int getUserVersion() {
    std::string versionQuery = "PRAGMA user_version";
    sqlite3_stmt* stmt;
    int currentVersion = 0;
    prepareStmt(versionQuery, stmt);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      currentVersion = sqlite3_column_int(stmt, 0);
    } else {
      return -1;
    }
    finalizeStmt(stmt);
    return currentVersion;
  }

 private:
  bool checkAndUpgradeDatabase() {
    int currentVersion = getUserVersion();
    LOG(INFO) << "currentVersion:" << currentVersion;
    if (currentVersion == -1) {
      return false;
    } else if (currentVersion == 0) {
      onCreate_();
    } else if (currentVersion < getUpgradeDatabaseVersion()) {
      onUpgrade_();
      setUserVersion(getUpgradeDatabaseVersion());
    }
    return true;
  }

  static int getUpgradeDatabaseVersion() { return 2; }

  void prepareStmt(const std::string& query, sqlite3_stmt*& stmt) {
    const char* err_msg = nullptr;
    if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, &err_msg) !=
        SQLITE_OK) {
      std::cerr << "Prepare statement error: " << err_msg << std::endl;
    }
  }

  void setUserVersion(int version) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "PRAGMA user_version = " + std::to_string(version);
    std::string errMsg;
    if (execSql(sql, &errMsg)) {
      LOG(ERROR) << "setUserVersion error";
    }
  }

  void finalizeStmt(sqlite3_stmt* stmt) { sqlite3_finalize(stmt); }
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SQLITE_HELPER_H
