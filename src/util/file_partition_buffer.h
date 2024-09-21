/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_FILE_PARTITION_BUFFER_H
#define BAZEL_TEMPLATE_UTIL_FILE_PARTITION_BUFFER_H

#include <memory>
#include <string>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/proto/config.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class FilePartitionBuffer {
 private:
  friend class folly::Singleton<FilePartitionBuffer>;
  FilePartitionBuffer() = default;

 public:
  using FileBufferList = std::list<std::string>;
  static std::shared_ptr<FilePartitionBuffer> Instance();

  bool Add(const std::string& sha256, int32_t partition_num,
           const std::string& content) {
    return true;
  }

  std::map<std::string, FileBufferList> buffers_;
  std::map<std::string, uint32_t> wrote_partition_num;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_FILE_PARTITION_BUFFER_H
