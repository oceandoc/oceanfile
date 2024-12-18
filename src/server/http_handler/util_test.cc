/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/server/http_handler/util.h"

#include "gtest/gtest.h"

namespace oceandoc {
namespace server {
namespace http_handler {

TEST(Util, GetName) {
  EXPECT_EQ(Util::GetName(" form-data; name=\"chunk\""), "chunk");
}

TEST(Util, HandleMultipart) {
  EXPECT_EQ(Util::GetName(" form-data; name=\"chunk\""), "chunk");
}

TEST(Util, ParseQueryString) {
  auto query_map =
      Util::ParseQueryString("https://www.baidu.com/?key1=value1&key2=value2");
  EXPECT_EQ(query_map.begin()->first, "key1");
  EXPECT_EQ(query_map.begin()->second, "value1");
  EXPECT_EQ(query_map.size(), 2);
}

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc
