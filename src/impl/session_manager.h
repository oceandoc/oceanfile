/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_SESSION_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_SESSION_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "src/common/defs.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class Session final {
 public:
  std::string user;
  int64_t last_update_time;
  std::string token;
};

class SessionManager final {
 private:
  friend class folly::Singleton<SessionManager>;
  SessionManager() {}

 public:
  static std::shared_ptr<SessionManager> Instance();

  ~SessionManager() {}

  bool Init() { return true; }

  std::string GenerateToken(const std::string& user) {
    auto token = util::Util::UUID();
    Session session;
    session.last_update_time = util::Util::CurrentTimeMillis();
    session.user = user;
    session.token = token;
    absl::base_internal::SpinLockHolder locker(&lock_);
    token_sessions_.emplace(token, session);
    user_sessions_.emplace(user, session);
    return token;
  }

  bool ValidateSession(const std::string& token, std::string* user) {
    if (token.empty()) {
      return false;
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = token_sessions_.find(token);
    if (it == token_sessions_.end()) {
      return false;
    }
    auto now = util::Util::CurrentTimeMillis();
    if (now - it->second.last_update_time >= common::SESSION_INTERVAL) {
      return false;
    }

    *user = it->second.user;
    return true;
  }

  bool UpdateSession(const std::string& user, const std::string& token,
                     std::string* new_token) {
    if (token.empty()) {
      return false;
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = token_sessions_.find(token);
    if (it == token_sessions_.end()) {
      LOG(INFO) << "token session not found";
      return false;
    }

    if (user != it->second.user) {
      LOG(INFO) << "user not same";
      return false;
    }

    auto now = util::Util::CurrentTimeMillis();
    if (now - it->second.last_update_time >= common::SESSION_INTERVAL) {
      LOG(INFO) << "expired";
      return false;
    }

    it->second.last_update_time = now;

    auto new_session = it->second;
    new_session.token = util::Util::UUID();

    user_sessions_[user] = new_session;
    token_sessions_.erase(it);
    token_sessions_[new_session.token] = new_session;
    *new_token = new_session.token;
    return true;
  }

  std::string QueryUserToken(const std::string& user) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto user_it = user_sessions_.find(user);
    if (user_it == user_sessions_.end()) {
      return "";
    }
    return user_it->second.token;
  }

  void KickoutByUser(const std::string& user) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto user_it = user_sessions_.find(user);
    if (user_it == user_sessions_.end()) {
      return;
    }

    auto token_it = token_sessions_.find(user_it->second.token);
    if (token_it == token_sessions_.end()) {
      LOG(ERROR) << "Cannot find user's token, this should nerver happen";
      return;
    }
    user_sessions_.erase(user_it);
    token_sessions_.erase(token_it);
  }

  void KickoutByToken(const std::string& token) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto token_it = token_sessions_.find(token);
    if (token_it == token_sessions_.end()) {
      return;
    }

    auto user_it = user_sessions_.find(token_it->second.user);
    if (user_it == user_sessions_.end()) {
      LOG(ERROR) << "Cannot find token's user, this should nerver happen";
      return;
    }

    user_sessions_.erase(user_it);
    token_sessions_.erase(token_it);
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::unordered_map<std::string, Session> token_sessions_;
  std::unordered_map<std::string, Session> user_sessions_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_SESSION_MANAGER_H
