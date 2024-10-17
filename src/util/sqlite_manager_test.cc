/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sqlite_manager.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(SqliteManager, ExecuteNonQuery) {
  EXPECT_EQ(SqliteManager::Instance()->Init("./data/user.db"), true);
}

}  // namespace util
}  // namespace oceandoc
