/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H
#define BAZEL_TEMPLATE_UTIL_SQLITE_ROW_H

#include <string>

#include "external/sqlite/sqlite3.h"
#include "glog/logging.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace util {

struct UsersRow {
  int32_t id;
  std::string user;
  std::string salt;
  std::string password;

  bool Extract(sqlite3_stmt* stmt) {
    salt.reserve(common::kSaltSize * 2);
    for (int i = 0; i < common::kSaltSize * 2; ++i) {
      auto c = sqlite3_column_text(stmt, 0)[i];
      std::bitset<8> binary(c);  // 8 bits for unsigned char
      LOG(INFO) << binary.to_string();
      LOG(INFO) << static_cast<unsigned int>(c);
    }

    salt.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
                common::kSaltSize * 2);
    password.reserve(common::kDerivedKeySize * 2);
    password.append(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                    common::kDerivedKeySize * 2);
    return true;
  }
};

struct MetaRow {
  int32_t id;
  int32_t version;
  bool Extract(sqlite3_stmt* /*stmt*/) { return true; }
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
