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
  int64_t photo_taken_time;
  int32_t type;
  std::string file_name;
  std::string owner;
  std::string file_hash;
  std::string live_photo_video_hash;
  std::string thumb_hash;
  bool Extract(sqlite3_stmt* /*stmt*/) { return true; }
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H
