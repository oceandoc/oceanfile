/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_COMMON_STRUCTS_H
#define BAZEL_TEMPLATE_COMMON_STRUCTS_H

namespace oceandoc {
namespace common {

enum SendStatus {
  SUCCESS,
  RETRING,
  TOO_MANY_RETRY,
};

}  // namespace common
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_COMMON_STRUCTS_H
