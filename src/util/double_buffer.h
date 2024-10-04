/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_DOUBLE_BUFFER_H
#define BAZEL_TEMPLATE_UTIL_DOUBLE_BUFFER_H

#include <string>

#include "absl/base/internal/spinlock.h"
#include "glog/logging.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

template <typename T>
class DoubleBuffer {
 public:
  DoubleBuffer() : pos_(0) {}

  void Insert(const T& items) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    for (const auto& item : items) {
      holder_[pos_].emplace_back(item);
    }
  }

  bool Dump(const std::string& path) {
    Swap();

    std::string content;
    std::string json;
    absl::base_internal::SpinLockHolder locker(&lock_);
    for (const auto& item : holder_[1 - pos_]) {
      Util::MessageToJson(item, &json);
      content.append(json);
#if defined(_WIN32)
      content.append("\r\n");
#else
      content.append("\n");
#endif
    }
    Util::WriteToFile(path, content, true);
    return true;
  }

  void Clear() {
    absl::base_internal::SpinLockHolder locker(&lock_);
    holder_[0].clear();
    holder_[1].clear();
  }

 private:
  void Swap() {
    absl::base_internal::SpinLockHolder locker(&lock_);
    pos_ = 1 - pos_;
    if (!holder_[pos_].empty()) {
      LOG(ERROR) << "This shouldn't happen";
      holder_[pos_].clear();
    }
  }

 private:
  T holder_[2];
  int pos_;
  mutable absl::base_internal::SpinLock lock_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_DOUBLE_BUFFER_H
