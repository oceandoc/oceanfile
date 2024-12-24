/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(SqliteManager, ExecuteNonQuery) {
  std::string home_dir = "/usr/local/fstation";
  EXPECT_EQ(SqliteManager::Instance()->Init(home_dir), true);
}

}  // namespace util
}  // namespace oceandoc
