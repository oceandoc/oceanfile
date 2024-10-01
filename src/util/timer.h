
/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_TIMER_H
#define BAZEL_TEMPLATE_UTIL_TIMER_H

#include "src/util/util.h"

namespace oceandoc {
namespace util {

struct Timer {
  Timer() : start_(Util::CurrentTimeMillis()) {}
  ~Timer() { LOG(INFO) << "cost " << Util::CurrentTimeMillis() - start_; }

 private:
  int64_t start_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_TIMER_H
