/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/test/test_util.h"

namespace oceandoc {
namespace test {

TEST(Util, Workspace) { EXPECT_EQ(Util::Workspace(), "oceandoc"); }
TEST(Util, WorkspaceRoot) { LOG(INFO) << Util::WorkspaceRoot(); }
TEST(Util, ExecRoot) { LOG(INFO) << Util::ExecRoot(); }

}  // namespace test
}  // namespace oceandoc
