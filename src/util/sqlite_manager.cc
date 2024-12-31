/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_manager.h"

namespace oceandoc {
namespace util {

static folly::Singleton<SqliteManager> sql_lite;
SqliteBinder SqliteManager::DoNothing = [](sqlite3_stmt* /*stmt*/) -> bool {
  return true;
};

std::shared_ptr<SqliteManager> SqliteManager::Instance() {
  return sql_lite.try_get();
}

}  // namespace util
}  // namespace oceandoc
