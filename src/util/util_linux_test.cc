/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util_linux.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(UtilLinux, PartitionUUID) {
  LOG(INFO) << UtilLinux::PartitionUUID("/zfs");
  LOG(INFO) << UtilLinux::PartitionUUID("/root");
}

}  // namespace util
}  // namespace oceandoc
