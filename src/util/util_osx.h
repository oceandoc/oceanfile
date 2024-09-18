/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_UTIL_OSX_H
#define BAZEL_TEMPLATE_UTIL_UTIL_OSX_H

#include <string_view>

namespace oceandoc {
namespace util {

class UtilOsx final {
 public:
  static void PartitionUUID(std::string_view path);
  static std::string Partition(std::string_view path);
  static bool SetFileInvisible(std::string_view path);
};

}  // namespace util
}  // namespace oceandoc

#endif /* BAZEL_TEMPLATE_UTIL_UTIL_OSX_H */
