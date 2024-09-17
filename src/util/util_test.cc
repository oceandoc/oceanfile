/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(Util, DetailTimeStr) {
  EXPECT_EQ(Util::DetailTimeStr(1646397312000),
            "2022-03-04T20:35:12.000+08:00");
  LOG(INFO) << Util::DetailTimeStr(1646397312000);
}

TEST(Util, UpdateTime) {
  const auto& path = "/usr/include/x86_64-linux-gnu/openmpi";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::UpdateTime(path), 1646397312000);
  LOG(INFO) << Util::UpdateTime(path);
}

TEST(Util, CreateTime) {
  auto path = "/usr/include/x86_64-linux-gnu/openmpi";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::CreateTime(path), 1720900552965);
  LOG(INFO) << Util::CreateTime(path);

  path = "/root/src/Dr.Q/oceanfile/txt";
  EXPECT_EQ(Util::CreateTime(path), 1726575142368);
  LOG(INFO) << Util::CreateTime(path);
}

}  // namespace util
}  // namespace oceandoc
