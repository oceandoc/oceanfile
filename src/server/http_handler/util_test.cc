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

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc
