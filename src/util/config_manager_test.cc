/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/config_manager.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(ConfigManager, Init) {
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  EXPECT_TRUE(ConfigManager::Instance()->Init(
      home_dir, home_dir + "/conf/server_base_config.json"));
  LOG(INFO) << ConfigManager::Instance()->ToString();
}

}  // namespace util
}  // namespace oceandoc
