/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_helper.h"

namespace oceandoc {
namespace util {

static folly::Singleton<SqliteHelper> sql_lite;

std::shared_ptr<SqliteHelper> SqliteHelper::Instance() {
  return sql_lite.try_get();
}

}  // namespace util
}  // namespace oceandoc
