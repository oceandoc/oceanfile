/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_USER_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_USER_MANAGER_H

#include <memory>
#include <string>
#include <vector>

#include "folly/Singleton.h"
#include "src/impl/session_manager.h"
#include "src/util/sqlite_manager.h"
#include "src/util/sqlite_row.h"
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

    int affect_rows = 0;
    std::string err_msg;
    std::string sql =
        R"(INSERT OR IGNORE INTO users
             (user, salt, password, create_time, update_time)
             VALUES (?, ?, ?, ?, ?);)";
    SqliteBinder bind_callback = [&salt, &hashed_password,
                                  &user](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 3, hashed_password.c_str(), -1,
                            SQLITE_STATIC)) {
        return false;
      }
      int64_t now = util::Util::CurrentTimeMillis();
      if (sqlite3_bind_int64(stmt, 4, now)) {
        return false;
      }
      if (sqlite3_bind_int64(stmt, 5, now)) {
        return false;
      }
      return true;
    };
    auto ret = util::SqliteManager::Instance()->Insert(sql, &affect_rows,
                                                       &err_msg, bind_callback);
    if (ret) {
      return Err_Fail;
    }

    if (affect_rows > 0) {
      *token = SessionManager::Instance()->GenerateToken(user);
      return Err_Success;
    } else {
      LOG(ERROR) << "No records were updated. user: " << user
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
      int affect_rows = 0;
      std::string err_msg;
      std::string sql = "DELETE FROM users WHERE user = ?;";
      SqliteBinder bind_callback =
          [&to_delete_user](sqlite3_stmt* stmt) -> bool {
        if (sqlite3_bind_text(stmt, 1, to_delete_user.c_str(), -1,
                              SQLITE_STATIC)) {
          return false;
        }
        return true;
      };
      auto ret = util::SqliteManager::Instance()->Delete(
          sql, &affect_rows, &err_msg, bind_callback);
      if (ret) {
        return Err_Fail;
      }
      if (affect_rows > 0) {
        SessionManager::Instance()->KickoutByToken(token);
        return Err_Success;
      } else {
        LOG(ERROR) << "No records were deleted. user: " << to_delete_user
                   << " may not exist";
      }
    }
    return Err_User_not_exists;
  }

  int32_t SelectUser(const std::string& user, std::vector<util::UsersRow>* rows,
                     std::string* err_msg) {
    std::string sql =
        R"(SELECT user, salt, password, create_time, update_time
         FROM users
         WHERE user = ?;)";
    SqliteBinder bind_callback = [&user](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 1, user.c_str(), user.size(),
                            SQLITE_STATIC)) {
        return false;
      }
      return true;
    };
    auto ret = util::SqliteManager::Instance()->Select<util::UsersRow>(
        sql, err_msg, bind_callback, rows);
    if (ret) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t VerifyPassword(const std::string& user, const std::string& password) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (password.size() > 64 || password.size() < 8) {
      return Err_User_passwd_error;
    }

    std::string err_msg;
    std::vector<util::UsersRow> rows;
    auto ret = SelectUser(user, &rows, &err_msg);
    if (ret) {
      return ret;
    }

    if (!rows.empty()) {
      const auto& user = rows.front();
      if (util::Util::VerifyPassword(password, user.salt, user.password)) {
        return Err_Success;
      } else {
        return Err_User_passwd_error;
      }
    }

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
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (token.empty()) {
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

    std::string err_msg;
    std::vector<util::UsersRow> rows;
    auto ret = SelectUser(user, &rows, &err_msg);
    if (ret) {
      return ret;
    }

    if (!rows.empty()) {
      return Err_Success;
    }

    return Err_User_not_exists;
  }

  int32_t ChangePassword(const std::string& user,
                         const std::string& old_password,
                         const std::string& new_password, std::string* token) {
    if (user.size() > 64 || user.empty()) {
      return Err_User_name_error;
    }

    if (new_password.size() > 64 || new_password.size() < 8) {
      return Err_User_passwd_error;
    }

    auto ret = VerifyPassword(user, old_password);
    if (ret) {
      return ret;
    }

    std::string salt = util::Util::GenerateSalt();
    std::string hashed_password;
    if (!util::Util::HashPassword(new_password, salt, &hashed_password)) {
      return Err_User_passwd_error;
    }

    int affect_rows = 0;
    std::string err_msg;
    std::string sql = "UPDATE users SET salt = ?, password = ? WHERE user = ?;";
    SqliteBinder bind_callback =
        [&user, &salt, &hashed_password](sqlite3_stmt* stmt) -> bool {
      if (sqlite3_bind_text(stmt, 3, user.c_str(), user.size(),
                            SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 1, salt.c_str(), salt.size(),
                            SQLITE_STATIC)) {
        return false;
      }
      if (sqlite3_bind_text(stmt, 2, hashed_password.c_str(),
                            hashed_password.size(), SQLITE_STATIC)) {
        return false;
      }
      return true;
    };
    ret = util::SqliteManager::Instance()->Update(sql, &affect_rows, &err_msg,
                                                  bind_callback);
    if (ret) {
      return Err_Fail;
    }

    if (affect_rows > 0) {
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
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_USER_MANAGER_H
