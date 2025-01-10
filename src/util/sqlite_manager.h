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
#include "src/util/sqlite_row.h"
#include "src/util/util.h"

using SqliteBinder = std::function<bool(sqlite3_stmt* stmt)>;

namespace oceandoc {
namespace util {

class SqliteManager final {
  FRIEND_TEST(SqliteManager, Update);

 private:
  friend class folly::Singleton<SqliteManager>;
  SqliteManager() {}

 public:
  static SqliteBinder DoNothing;
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
    db_path_ = home_dir + "/data/fstation.db";

    if (!Util::Exists(home_dir + "/data")) {
      if (!Util::Mkdir(home_dir + "/data")) {
        return false;
      }
    }
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

  int32_t ExecuteNonQuery(const std::string& sql, std::string* err_msg) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    char* raw_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      sqlite3_close(db);
      return Err_Fail;
    }

    conns_.PushBack(db);
    return Err_Success;
  }

  template <class RowType>
  int32_t Select(const std::string& sql, std::string* err_msg,
                 const SqliteBinder& bind_callback,
                 std::vector<RowType>* rows) {
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

    if (!bind_callback(stmt)) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      RowType row;
      row.Extract(stmt);
      rows->push_back(row);
    }

    if (rc != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    PutConn(db);
    return Err_Success;
  }

  int32_t Insert(const std::string& sql, int32_t* affect_rows,
                 std::string* err_msg, const SqliteBinder& bind_callback) {
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

    if (!bind_callback(stmt)) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    *affect_rows = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    PutConn(db);
    return Err_Success;
  }

  // need call sqlite3_step and sqlite3_reset in bind_callback
  int32_t InsertBatch(const std::string& sql, std::string* err_msg,
                      const SqliteBinder& bind_callback) {
    sqlite3* db = GetConn();
    if (db == nullptr) {
      return Err_Fail;
    }

    char* raw_msg = nullptr;
    std::string query = "BEGIN TRANSACTION;";
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      sqlite3_close(db);
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    if (!bind_callback(stmt)) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    sqlite3_finalize(stmt);

    query = "COMMIT;";
    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &raw_msg);
    if (rc != SQLITE_OK) {
      err_msg->append(raw_msg);
      sqlite3_free(raw_msg);
      sqlite3_close(db);
      return Err_Fail;
    }

    PutConn(db);
    return Err_Success;
  }

  int32_t Update(const std::string& sql, int32_t* affect_rows,
                 std::string* err_msg, const SqliteBinder& bind_callback) {
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

    if (!bind_callback(stmt)) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    *affect_rows = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    PutConn(db);
    return Err_Success;
  }

  int32_t Delete(const std::string& sql, int32_t* affect_rows,
                 std::string* err_msg, const SqliteBinder& bind_callback) {
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

    if (!bind_callback(stmt)) {
      err_msg->append(sqlite3_errmsg(db));
      return Err_Fail;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      err_msg->append(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return Err_Fail;
    }

    *affect_rows = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    PutConn(db);
    return Err_Success;
  }

  int32_t SelectFile(const std::string& file_hash,
                     std::vector<util::FilesRow>* rows, std::string* err_msg) {
    std::string sql = "SELECT * FROM files WHERE file_hash = ?;";
    SqliteBinder bind_callback = [&file_hash](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 1, file_hash.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }

      return true;
    };
    auto ret = Select<util::FilesRow>(sql, err_msg, bind_callback, rows);
    if (ret) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t SelectFiles(const std::string& repo_dir,
                      std::vector<util::FilesRow>* rows, std::string* err_msg) {
    std::string sql = "SELECT * FROM files WHERE repo_dir = ?;";
    SqliteBinder bind_callback = [&repo_dir](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 1, repo_dir.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }

      return true;
    };
    auto ret = Select<util::FilesRow>(sql, err_msg, bind_callback, rows);
    if (ret) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t SelectMediaFiles(const int32_t offset, const int32_t limit,
                           std::vector<util::FilesRow>* rows,
                           std::string* err_msg) {
    std::string sql =
        "SELECT * FROM files WHERE repo_dir = /media LIMIT ? OFFSET ?;";
    SqliteBinder bind_callback = [&offset, &limit](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_int(stmt, 1, limit)) {
        return false;
      }
      if (sqlite3_bind_int(stmt, 2, offset)) {
        return false;
      }

      return true;
    };
    auto ret = Select<util::FilesRow>(sql, err_msg, bind_callback, rows);
    if (ret) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t InsertFile(const util::FilesRow& row, std::string* err_msg) {
    std::string sql =
        "INSERT OR IGNORE INTO files (local_id, device_id, repo_dir, "
        "file_hash, type, file_name, owner, taken_time, video_hash, "
        "cover_hash, thumb_hash) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    SqliteBinder bind_callback = [&row](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 1, row.local_id.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 1, row.device_id.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 1, row.repo_dir.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.file_hash.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_int(stmt, 2, row.type)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.file_name.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.owner.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_int(stmt, 2, row.taken_time)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.video_hash.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.cover_hash.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }

      if (sqlite3_bind_text(stmt, 1, row.thumb_hash.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }

      return true;
    };
    int32_t affect_rows = 0;
    auto ret = Insert(sql, &affect_rows, err_msg, bind_callback);
    if (ret) {
      return Err_Fail;
    }
    return Err_Success;
  }

 private:
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
    std::string sql = R"(CREATE TABLE IF NOT EXISTS users (
                           id INTEGER PRIMARY KEY AUTOINCREMENT,
                           user TEXT UNIQUE,
                           salt TEXT,
                           password TEXT,
                           create_time INTEGER,
                           update_time INTEGER
                         );)";

    if (ExecuteNonQuery(sql, &error_msg)) {
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
               repo_dir TEXT,
               file_hash TEXT,
               type INTEGER,
               file_name TEXT DEFAULT '',
               owner TEXT DEFAULT '',
               taken_time INTEGER DEFAULT -1,
               video_hash TEXT DEFAULT '',
               cover_hash TEXT DEFAULT '',
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

  bool InitAdminUser() {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql =
        R"(INSERT OR IGNORE INTO users
             (user, salt, password, create_time, update_time)
             VALUES (?, ?, ?, ?, ?);)";
    SqliteBinder bind_callback = [](sqlite3_stmt* stmt) -> bool {
      std::string salt = "452c0306730b0f3ac3086d4f62effc20";
      std::string password =
          "e64de2fcaef0b98d035c3c241e4f8fda32f3b09067ef0f1b1706869a54f9d3b7";
      if (sqlite3_bind_text(stmt, 1, "admin", -1, SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_TRANSIENT)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT)) {
        return false;
      }

      int64_t now = Util::CurrentTimeMillis();
      if (sqlite3_bind_int64(stmt, 4, now)) {
        return false;
      }
      if (sqlite3_bind_int64(stmt, 5, now)) {
        return false;
      }
      return true;
    };

    if (Insert(sql, &affect_rows, &err_msg, bind_callback)) {
      LOG(ERROR) << "Init admin error: " << err_msg;
      return false;
    }

    if (affect_rows <= 0) {
      LOG(ERROR) << "Init admin error, already exists";
      return false;
    }

    LOG(INFO) << "Init admin success";
    return true;
  }

  bool InitVersion() {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql = R"(INSERT INTO meta (id, version) VALUES (?, ?);)";
    SqliteBinder bind_callback = [](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_int(stmt, 1, 1)) {
        return false;
      }
      if (sqlite3_bind_int(stmt, 2, 1)) {
        return false;
      }
      return true;
    };

    if (Insert(sql, &affect_rows, &err_msg, bind_callback)) {
      LOG(ERROR) << "Insert to meta error: " << err_msg;
      return false;
    }

    if (affect_rows > 0) {
      LOG(INFO) << "Init version success";
      return true;
    } else {
      LOG(ERROR) << "Already exists version";
    }
    return false;
  }

  bool UpdateVersion(const int32_t version) {
    int affect_rows = 0;
    std::string err_msg;
    std::string sql = R"(UPDATE meta SET version = ? where id = 1;)";
    SqliteBinder bind_callback = [version](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_int(stmt, 1, version)) {
        return false;
      }
      return true;
    };

    if (Update(sql, &affect_rows, &err_msg, bind_callback)) {
      LOG(ERROR) << "Update version error: " << err_msg;
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
    std::string sql = R"(SELECT version FROM meta WHERE id = 1)";
    SqliteBinder bind_callback = [](sqlite3_stmt* /*stmt*/) -> bool {
      return true;
    };
    std::vector<MetaRow> rows;
    if (Select<MetaRow>(sql, &err_msg, bind_callback, &rows)) {
      LOG(ERROR) << "Select version error: " << err_msg;
      return 0;
    }

    if (!rows.empty()) {
      return rows.front().version;
    } else {
      LOG(ERROR) << "Select version empty";
    }
    return 0;
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
