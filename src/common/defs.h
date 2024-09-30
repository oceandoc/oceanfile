/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_COMMON_DEFS_H
#define BAZEL_TEMPLATE_COMMON_DEFS_H

#include <fstream>
#include <string>

namespace oceandoc {
namespace common {

enum SendStatus {
  SUCCESS,
  RETRING,
  TOO_MANY_RETRY,
  FATAL,
  FAIL,
};

const std::string CONFIG_DIR = ".Dr.Q.config";
const std::string REPOS_CONFIG_FILE = "./data/repos.json";
constexpr int64_t BUFFER_SIZE = 64 * 1024 * 1024 * 8;    // 64MB, unit:byte
constexpr int64_t BUFFER_SIZE_BYTES = 64 * 1024 * 1024;  // 64MB unit:Bytes

constexpr int64_t MAX_GRPC_MSG_SIZE = 2 * 64 * 1024 * 1024 * 8;  // 128MB
constexpr double TRACING_SAMPLER_PROBALITITY = 0.01;             // 1 Percent

struct UploadContext {
  std::ifstream file;
  size_t remaining;
};

struct FileAttr {
  std::string path;
  std::string sha256;
  std::string enc_sha256;
  int64_t size;
  int32_t partition_num;
  std::string ToString() {
    std::string content;
    content.append("path: ");
    content.append(path);
    content.append(", ");

    content.append("sha256: ");
    content.append(sha256);
    content.append(", ");

    content.append("enc_sha256: ");
    content.append(enc_sha256);
    content.append(", ");

    content.append("size: ");
    content.append(std::to_string(size));
    content.append(", ");

    content.append("partition_num: ");
    content.append(std::to_string(partition_num));
    return content;
  }
};

}  // namespace common
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_COMMON_DEFS_H
