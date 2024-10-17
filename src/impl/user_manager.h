/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_USER_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_USER_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "src/impl/session_manager.h"
#include "src/util/sqlite_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class UserManager final {
 private:
  friend class folly::Singleton<UserManager>;
  UserManager() {}

 public:
  static std::shared_ptr<UserManager> Instance();

  ~UserManager() {}

  bool Init() {
    if (!util::SqliteManager::Instance()->Init()) {
      return false;
    }
    return true;
  }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
  }

  int32_t RegisterUser(const std::string& username, const std::string& password,
                       std::string* token) {
    std::string salt = util::Util::GenerateSalt();
    std::string passwd_hash;
    std::string hashed_password;
    if (!util::Util::HashPassword(password, salt, &hashed_password)) {
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    util::SqliteManager::Instance()->PrepareStatement(
        "INSERT INTO users (username, salt, password) VALUES (?, ?, ?);", stmt);
    if (!stmt) {
      return Err_Sql_prepare_error;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hashed_password.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      return Err_Sql_execute_error;
    }
    sqlite3_finalize(stmt);

    *token = SessionManager::Instance()->GenerateToken(username);
    return Err_Success;
  }

  int32_t DeleteUser(const std::string& to_delete_user,
                     const std::string& token) {
    std::string login_user;
    if (!SessionManager::Instance()->ValidateSession(token, &login_user)) {
      return Err_User_session_error;
    }

    if (to_delete_user == "admin") {
      LOG(ERROR) << "Cannot delete admin";
      return Err_User_invalid_name;
    }

    if (login_user == "admin" || login_user == to_delete_user) {
      sqlite3_stmt* stmt = nullptr;
      util::SqliteManager::Instance()->PrepareStatement(
          "INSERT INTO users (username, salt, password) VALUES (?, ?, ?);",
          stmt);
      if (!stmt) {
        return Err_Sql_prepare_error;
      }

      // sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
      // sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_STATIC);
      // sqlite3_bind_text(stmt, 3, hashed_password.c_str(), -1, SQLITE_STATIC);

      if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Err_Sql_execute_error;
      }
      SessionManager::Instance()->KickoutByToken(token);
      sqlite3_finalize(stmt);
    }
    return Err_User_invalid_name;
  }

  bool LoginUser(const std::string& username, const std::string& password,
                 std::string* token) {
    sqlite3_stmt* stmt = nullptr;
    util::SqliteManager::Instance()->PrepareStatement(
        "SELECT salt, password FROM users WHERE username = ?;", stmt);
    if (!stmt) {
      return Err_Sql_prepare_error;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string stored_salt =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      std::string stored_hashed_password =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      sqlite3_finalize(stmt);

      if (util::Util::VerifyPassword(password, stored_salt,
                                     stored_hashed_password)) {
        *token = SessionManager::Instance()->GenerateToken(username);
        return Err_Success;
      } else {
        return Err_User_invalid_passwd;
      }
    }

    sqlite3_finalize(stmt);
    return Err_User_invalid_name;
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_USER_MANAGER_H
