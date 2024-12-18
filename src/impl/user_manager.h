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

  bool Init() { return true; }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
  }

  int32_t UserRegister(const std::string& user, const std::string& password,
                       std::string* token) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (password.size() > 64 || password.size() < 8) {
      return Err_User_passwd_error;
    }

    std::string salt = util::Util::GenerateSalt();
    std::string hashed_password;
    if (!util::Util::HashPassword(password, salt, &hashed_password)) {
      return Err_Fail;
    }

    sqlite3_stmt* stmt = nullptr;
    auto ret = util::SqliteManager::Instance()->PrepareStatement(
        "INSERT OR IGNORE INTO users (user, salt, password) VALUES (?, ?, ?);",
        &stmt);
    if (ret) {
      return Err_Fail;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, salt.c_str(), salt.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hashed_password.c_str(), hashed_password.size(),
                      SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      return Err_Fail;
    }
    sqlite3_finalize(stmt);

    int changes = util::SqliteManager::Instance()->AffectRows();
    if (changes > 0) {
      *token = SessionManager::Instance()->GenerateToken(user);
      return Err_Success;
    } else {
      LOG(ERROR) << "No records were updated. user '" << user
                 << " may exist already";
    }
    return Err_User_exists;
  }

  int32_t UserDelete(const std::string& login_user,
                     const std::string& to_delete_user,
                     const std::string& token) {
    if (to_delete_user.size() > 64 || to_delete_user.empty()) {
      return Err_User_name_error;
    }

    if (to_delete_user == "admin") {
      LOG(ERROR) << "Cannot delete admin";
      return Err_User_name_error;
    }

    if (login_user == "admin" || login_user == to_delete_user) {
      sqlite3_stmt* stmt = nullptr;
      auto ret = util::SqliteManager::Instance()->PrepareStatement(
          "DELETE FROM users WHERE user = ?;", &stmt);
      if (ret) {
        return Err_Fail;
      }
      sqlite3_bind_text(stmt, 1, to_delete_user.c_str(), to_delete_user.size(),
                        SQLITE_STATIC);
      if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Err_Fail;
      }
      sqlite3_finalize(stmt);
      int changes = util::SqliteManager::Instance()->AffectRows();
      if (changes > 0) {
        SessionManager::Instance()->KickoutByToken(token);
        return Err_Success;
      } else {
        LOG(ERROR) << "No records were deleted. user '" << to_delete_user
                   << " may not exist";
      }
    }
    return Err_User_not_exists;
  }

  int32_t VerifyPassword(const std::string& user, const std::string& password) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (password.size() > 64 || password.size() < 8) {
      return Err_User_passwd_error;
    }

    sqlite3_stmt* stmt = nullptr;
    util::SqliteManager::Instance()->PrepareStatement(
        "SELECT salt, password FROM users WHERE user = ?;", &stmt);
    if (!stmt) {
      return Err_Fail;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string salt;
      salt.reserve(util::Util::kSaltSize);
      salt.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
                  util::Util::kSaltSize);
      std::string hashed_password;
      hashed_password.reserve(util::Util::kDerivedKeySize);
      hashed_password.append(
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
          util::Util::kDerivedKeySize);
      sqlite3_finalize(stmt);
      if (util::Util::VerifyPassword(password, salt, hashed_password)) {
        return Err_Success;
      } else {
        return Err_User_passwd_error;
      }
    }

    sqlite3_finalize(stmt);
    return Err_User_not_exists;
  }

  int32_t UserLogin(const std::string& user, const std::string& password,
                    std::string* token) {
    auto ret = VerifyPassword(user, password);
    if (ret) {
      return ret;
    }
    *token = SessionManager::Instance()->GenerateToken(user);
    return Err_Success;
  }

  int32_t UserValidateSession(const std::string& user,
                              const std::string& token) {
    if (user.empty() || token.empty()) {
      LOG(ERROR) << "user name or token empty";
      return Err_User_session_error;
    }

    std::string session_user;
    if (!impl::SessionManager::Instance()->ValidateSession(token,
                                                           &session_user)) {
      LOG(ERROR) << "token invalid";
      return Err_User_session_error;
    }
    if (session_user.empty() || session_user != user) {
      LOG(ERROR) << "user mismatch";
      return Err_User_session_error;
    }
    return Err_Success;
  }

  std::string UserQueryToken(const std::string& user) {
    if (user.empty()) {
      LOG(ERROR) << "user name empty";
      return "";
    }

    return impl::SessionManager::Instance()->QueryUserToken(user);
  }

  int32_t UserLogout(const std::string& token) {
    SessionManager::Instance()->KickoutByToken(token);
    return Err_Success;
  }

  int32_t UserExists(const std::string& user) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    sqlite3_stmt* stmt = nullptr;
    util::SqliteManager::Instance()->PrepareStatement(
        "SELECT salt, password FROM users WHERE user = ?;", &stmt);
    if (!stmt) {
      return Err_Fail;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), user.size(), SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string hashed_password;
      hashed_password.reserve(util::Util::kDerivedKeySize);
      hashed_password.append(
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
          util::Util::kDerivedKeySize);
      sqlite3_finalize(stmt);
      return Err_User_exists;
    }

    sqlite3_finalize(stmt);
    return Err_User_not_exists;
  }

  int32_t ChangePassword(const std::string& user,
                         const std::string& old_password,
                         const std::string& password, std::string* token) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (password.size() > 64 || password.size() < 8) {
      return Err_User_passwd_error;
    }

    auto ret = VerifyPassword(user, old_password);
    if (ret) {
      return ret;
    }

    std::string salt = util::Util::GenerateSalt();
    std::string hashed_password;
    if (!util::Util::HashPassword(password, salt, &hashed_password)) {
      return Err_User_passwd_error;
    }

    sqlite3_stmt* stmt = nullptr;
    ret = util::SqliteManager::Instance()->PrepareStatement(
        "UPDATE users SET salt = ?, password = ? WHERE user = ?;", &stmt);
    if (ret) {
      return Err_Fail;
    }

    sqlite3_bind_text(stmt, 3, user.c_str(), user.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 1, salt.c_str(), salt.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password.c_str(), hashed_password.size(),
                      SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      return Err_Fail;
    }
    sqlite3_finalize(stmt);

    int changes = util::SqliteManager::Instance()->AffectRows();
    if (changes > 0) {
      *token = SessionManager::Instance()->GenerateToken(user);
      return Err_Success;
    } else {
      LOG(ERROR) << "No records were updated. user '" << user
                 << " may not exist.";
    }
    return Err_Fail;
  }

  int32_t UpdateToken(const std::string& user, const std::string& token,
                      std::string* new_token) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (SessionManager::Instance()->UpdateSession(user, token, new_token)) {
      return Err_Success;
    }

    return Err_Fail;
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
