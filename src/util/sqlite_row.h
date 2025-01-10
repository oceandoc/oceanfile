/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H
#define BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H

#include <string>

#include "external/sqlite/sqlite3.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

struct UsersRow {
  int32_t id;
  std::string user;
  std::string salt;
  std::string password;
  int64_t create_time;
  int64_t update_time;

  bool Extract(sqlite3_stmt* stmt) {
    user.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    salt.reserve(common::kSaltSize * 2);
    salt.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                common::kSaltSize * 2);
    password.reserve(common::kDerivedKeySize * 2);
    password.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
                    common::kDerivedKeySize * 2);
    create_time = sqlite3_column_int64(stmt, 3);
    update_time = sqlite3_column_int64(stmt, 4);
    return true;
  }
};

struct MetaRow {
  int32_t id;
  int32_t version;
  bool Extract(sqlite3_stmt* stmt) {
    version = sqlite3_column_int(stmt, 0);
    return true;
  }
};

struct FilesRow {
  int32_t id;
  std::string local_id;
  std::string device_id;
  std::string repo_dir;
  std::string file_hash;
  int32_t type;
  std::string file_name;
  std::string owner;
  int64_t taken_time;
  std::string video_hash;
  std::string cover_hash;
  std::string thumb_hash;
  bool Extract(sqlite3_stmt* stmt) {
    id = sqlite3_column_int(stmt, 0);
    local_id.append((const char*)sqlite3_column_text(stmt, 1));
    device_id.append((const char*)sqlite3_column_text(stmt, 2));
    repo_dir.append((const char*)sqlite3_column_text(stmt, 3));
    file_hash.append((const char*)sqlite3_column_text(stmt, 4));
    type = sqlite3_column_int(stmt, 5);
    file_name.append((const char*)sqlite3_column_text(stmt, 6));
    owner.append((const char*)sqlite3_column_text(stmt, 7));
    taken_time = sqlite3_column_int64(stmt, 8);
    video_hash.append((const char*)sqlite3_column_text(stmt, 9));
    cover_hash.append((const char*)sqlite3_column_text(stmt, 10));
    thumb_hash.append((const char*)sqlite3_column_text(stmt, 11));
    return true;
  }
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H
