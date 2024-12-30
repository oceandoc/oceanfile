/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "external/sqlite/sqlite3.h"
#include "folly/Singleton.h"
#include "src/common/blocking_queue.h"
#include "src/common/error.h"
#include "src/common/sqlite_row.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class SqliteManager final {
 private:
  friend class folly::Singleton<SqliteManager>;
  SqliteManager() {}

 public:
  static std::shared_ptr<SqliteManager> Instance();

  ~SqliteManager() {
    while (conns_.Size() > 0) {
      sqlite3* db = nullptr;
      if (!conns_.PopBack(&db)) {
        continue;
      }
      if (db) {
        sqlite3_close(db);
      }
    }
  }

  bool Init(const std::string& home_dir) {
    db_path_ = home_dir + "/data/db";
    if (Util::Exists(db_path_) && Util::FileSize(db_path_) > 0) {
      if (!UpgradeTables()) {
        LOG(ERROR) << "Upgrade tables error";
        return false;
      }
    } else {
      if (!InitTables()) {
        LOG(ERROR) << "Init tables error";
        return false;
      }
    }

    for (int i = 0; i < kConnNum; i++) {
      sqlite3* db = CreateConn();
      if (db) {
        conns_.PushBack(db);
      }
    }
    return true;
  }

  bool UpgradeTables() {
    auto version = GetVersion();
    if (version == 0) {
      LOG(ERROR) << "Wrong version";
      return false;
    }
    if (version < kCurrentVersion) {
      // Upgrade logic
    }

    LOG(INFO) << "UpgradeTables Success";
    return true;
  }

  bool InitTables() {
    std::string error_msg;
    int affect_rows = 0;
    std::string sql = R"(CREATE TABLE IF NOT EXISTS users (
                           id INTEGER PRIMARY KEY AUTOINCREMENT,
                           user TEXT UNIQUE,
                           salt TEXT,
                           password TEXT
                         );)";

    if (ExecuteNonQuery(sql, &error_msg, &affect_rows)) {
      LOG(ERROR) << "Init users table error: " << error_msg;
      return false;
    }

    if (!InitAdminUser()) {
      return false;
    }

    error_msg.clear();
    sql = R"(CREATE TABLE IF NOT EXISTS meta (
               id INTEGER PRIMARY KEY AUTOINCREMENT,
               version INTEGER
             );)";
    if (ExecuteNonQuery(sql, &error_msg)) {
      LOG(ERROR) << "Init meta table error: " << error_msg;
      return false;
    }
    if (!InitVersion()) {
      return false;
    }

    error_msg.clear();
    sql = R"(CREATE TABLE IF NOT EXISTS files (
               id INTEGER PRIMARY KEY AUTOINCREMENT,
               local_id TEXT UNIQUE,
               device_id TEXT,
               photo_taken_time INTEGER DEFAULT -1,
               type INTEGER,
               file_name TEXT,
               owner TEXT,
               file_hash TEXT,
               live_photo_video_hash TEXT DEFAULT '',
               thumb_hash TEXT DEFAULT ''
             );)";

    if (ExecuteNonQuery(sql, &error_msg)) {
      LOG(ERROR) << "Init files table error: " << error_msg;
      return false;
    }

    LOG(INFO) << "InitTables Success";
    return true;
  }

  sqlite3* CreateConn() {
    sqlite3* db = nullptr;
    if (sqlite3_open(db_path_.c_str(), &db) != SQLITE_OK) {
      sqlite3_close(db);
      LOG(ERROR) << "Open database error: " << db_path_ << ", "
                 << sqlite3_errmsg(db);
      return nullptr;
    }
    return db;
  }

  sqlite3* GetConn() {
    sqlite3* db = nullptr;
    int cnt = 3;
    while (cnt-- >= 0) {
      if (!conns_.PopBack(&db)) {
        continue;
      }
      if (db) {
        return db;
      }
    }
    return CreateConn();
  }

  void PutConn(sqlite3* db) {
    if (db) {
      conns_.PushBack(db);
    }
  }

  template <class RowType>
  int32_t Select(const std::string& sql, std::string* err_msg,
                 const std::function<void(sqlite3_stmt* stmt)>& bind_callback,
                 std::vector<RowType>* rows) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    bind_callback(stmt);
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      RowType row;
      row.Extract(stmt);
      rows->emplace_back(row);
    }

    if (rc != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    PutConn(db);
    return Err_Success;
  }

  // 1. sql without parameters
  // 2. only execute once
  int32_t ExecuteNonQuery(const std::string& sql, std::string* err_msg,
                          int32_t* affect_rows = nullptr) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }
    char* raw_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      sqlite3_close(db);
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      return Err_Fail;
    }

    if (affect_rows) {
      *affect_rows = AffectRows(db);
    }
    conns_.PushBack(db);
    return Err_Success;
  }

  int32_t InsertBatch(
      const std::string& sql, std::string* err_msg,
      const std::function<void(sqlite3_stmt* stmt)>& bind_callback) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    char* raw_msg = nullptr;
    std::string query = "BEGIN TRANSACTION;";
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      sqlite3_close(db);
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    bind_callback(stmt);
    sqlite3_finalize(stmt);

    query = "COMMIT;";
    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      sqlite3_close(db);
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      return Err_Fail;
    }

    PutConn(db);
    return Err_Success;
  }

  int32_t Insert(const std::string& sql, int32_t* affect_rows,
                 std::string* err_msg,
                 const std::function<void(sqlite3_stmt* stmt)>& bind_callback) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    bind_callback(stmt);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    *affect_rows = AffectRows(db);
    sqlite3_finalize(stmt);
    PutConn(db);
    return Err_Success;
  }

  int32_t Update(const std::string& sql, int32_t* affect_rows,
                 std::string* err_msg,
                 const std::function<void(sqlite3_stmt* stmt)>& bind_callback) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    bind_callback(stmt);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    *affect_rows = AffectRows(db);
    sqlite3_finalize(stmt);
    PutConn(db);
    return Err_Success;
  }

  int32_t AffectRows(sqlite3* db) { return sqlite3_changes(db); }

 private:
  bool InitAdminUser() {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql =
        "INSERT OR IGNORE INTO users (user, salt, password) VALUES (?, ?, ?);";
    std::function<void(sqlite3_stmt * stmt)> bind_callback =
        [](sqlite3_stmt* stmt) {
          std::vector<uint8_t> salt_arr{0x45, 0x2c, 0x03, 0x06, 0x73, 0x0b,
                                        0x0f, 0x3a, 0xc3, 0x08, 0x6d, 0x4f,
                                        0x62, 0xef, 0xfc, 0x20};
          std::vector<uint8_t> hashed_password_arr{
              0x29, 0x9a, 0xe5, 0x3a, 0xb2, 0x2c, 0x08, 0x5a, 0x47, 0x96, 0xb5,
              0x91, 0x87, 0xd2, 0xb5, 0x4c, 0x21, 0x7e, 0x48, 0x30, 0xb4, 0xab,
              0xe4, 0xad, 0xe7, 0x9d, 0x7d, 0x8e, 0x6d, 0x90, 0xf5, 0x1a};
          std::string salt(salt_arr.begin(), salt_arr.end());
          std::string hashed_password(hashed_password_arr.begin(),
                                      hashed_password_arr.end());
          sqlite3_bind_text(stmt, 1, "admin", -1, SQLITE_STATIC);
          sqlite3_bind_text(stmt, 2, salt.c_str(), common::kSaltSize,
                            SQLITE_STATIC);
          sqlite3_bind_text(stmt, 3, hashed_password.c_str(),
                            common::kDerivedKeySize, SQLITE_STATIC);
        };
    auto ret = Insert(sql, &affect_rows, &err_msg, bind_callback);
    if (ret) {
      LOG(ERROR) << "Init admin error: " << err_msg;
      return false;
    }

    if (affect_rows <= 0) {
      LOG(ERROR) << "Init admin error: " << err_msg;
      return false;
    }
    LOG(ERROR) << "Init admin success";
    return true;
  }

  bool InitVersion() {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql = R"(INSERT INTO meta (id, version) VALUES (?, ?);)";
    std::function<void(sqlite3_stmt * stmt)> bind_callback =
        [](sqlite3_stmt* stmt) {
          sqlite3_bind_int(stmt, 1, 1);
          sqlite3_bind_int(stmt, 2, 1);
        };

    auto ret = Insert(sql, &affect_rows, &err_msg, bind_callback);
    if (ret) {
      LOG(ERROR) << "Insert to meta prepare error";
      return false;
    }

    if (affect_rows > 0) {
      LOG(INFO) << "Init version success";
      return true;
    } else {
      LOG(INFO) << "Already exists version";
    }
    return false;
  }

  bool UpdateVersion(const int32_t version) {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql = R"(UPDATE meta SET version = ? where id = 1;)";
    std::function<void(sqlite3_stmt * stmt)> bind_callback =
        [version](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, version); };
    auto ret = Update(sql, &affect_rows, &err_msg, bind_callback);
    if (ret) {
      LOG(ERROR) << "Update version error";
      return false;
    }

    if (affect_rows > 0) {
      LOG(INFO) << "Update version success";
      return true;
    }
    return false;
  }

  int32_t GetVersion() {
    std::string err_msg;
    std::string sql = R"(SELECT * FROM meta WHERE id = 1)";
    std::function<void(sqlite3_stmt * stmt)> bind_callback =
        [](sqlite3_stmt* /*stmt*/) {};
    std::vector<common::MetaRow> rows;
    auto ret = Select<common::MetaRow>(sql, &err_msg, bind_callback, &rows);
    if (ret) {
      LOG(ERROR) << "Select version error";
      return 0;
    }

    if (!rows.empty()) {
      return rows.front().version;
    } else {
      ret = 0;
      LOG(ERROR) << "Select version empty";
    }
    return ret;
  }

 private:
  static const int kConnNum = 100;
  common::BlockingQueue<sqlite3*> conns_;
  std::string db_path_;
  static const int kCurrentVersion = 1;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SQLITE_MANAGER_H
