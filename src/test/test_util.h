/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_TEST_TEST_UTIL_H
#define BAZEL_TEMPLATE_TEST_TEST_UTIL_H

#include "src/util/util.h"

namespace oceandoc {
namespace test {

class Util {
 private:
  Util() = delete;
  ~Util() = delete;

 public:
  static bool IsBazelRun() {
    auto runfile_dir = util::Util::GetEnv("TEST_SRCDIR");
    if (runfile_dir.has_value() && !runfile_dir.value().empty() &&
        !util::Util::Contain(runfile_dir.value(), "sandbox")) {
      return true;
    }
    return false;
  }

  static bool IsBazelTest() {
    auto runfile_dir = util::Util::GetEnv("TEST_SRCDIR");
    if (runfile_dir.has_value() &&
        util::Util::Contain(runfile_dir.value(), "sandbox")) {
      return true;
    }
    return false;
  }

  static std::string Workspace() {
    auto workspace = util::Util::GetEnv("TEST_WORKSPACE");
    if (workspace.has_value()) {
      return workspace.value();
    }
    return "";
  }

  static std::string WorkspaceRoot() {
    if (IsBazelRun()) {
      auto runfile_dir = util::Util::GetEnv("TEST_SRCDIR");
      auto pos = runfile_dir.value().find("execroot");
      return runfile_dir.value().substr(0, pos - 1);
    } else if (IsBazelTest()) {
      auto runfile_dir = util::Util::GetEnv("TEST_SRCDIR");
      auto pos = runfile_dir.value().find("sandbox");
      return runfile_dir.value().substr(0, pos - 1);
    }
    return "";
  }

  static std::string ExecRoot() {
    std::string workspace = WorkspaceRoot();
    if (workspace.empty()) {
      return "";
    }
    return workspace + "/execroot";
  }

  static std::string SandboxExecRoot() {
    auto runfile_dir = util::Util::GetEnv("TEST_SRCDIR");
    if (runfile_dir.has_value()) {
      auto pos = runfile_dir.value().find("execroot");
      return runfile_dir.value().substr(0, pos) + "execroot";
    }
    return "";
  }
};

}  // namespace test
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_TEST_TEST_UTIL_H
