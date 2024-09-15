/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_TOOLS_COMMON_H
#define BAZEL_TEMPLATE_TOOLS_COMMON_H

#include <filesystem>
#include <string>

namespace oceandoc {
namespace tools {

class Common {
 public:
  static bool ConfGen(const std::filesystem::path &origin_file,
                      std::string *out);

  static bool ConfReplace(const std::filesystem::path &origin_file,
                          const std::filesystem::path &final_file,
                          std::string *out);
  static bool ConfDiff(const std::filesystem::path &origin_file,
                       const std::filesystem::path &final_file,
                       std::string *out);
};

}  // namespace tools
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_TOOLS_COMMON_H
