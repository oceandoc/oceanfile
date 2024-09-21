/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/file_partition_buffer.h"

namespace oceandoc {
namespace util {

static folly::Singleton<FilePartitionBuffer> file_partition_buffer;

std::shared_ptr<FilePartitionBuffer> FilePartitionBuffer::Instance() {
  return file_partition_buffer.try_get();
}

}  // namespace util
}  // namespace oceandoc
