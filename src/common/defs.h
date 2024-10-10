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
  SUCCESS = 0,
  RETRING,
  TOO_MANY_RETRY,
  FATAL,
  FAIL,
};

enum HashMethod {
  Hash_NONE = 0,
  Hash_CRC32,
  Hash_MD5,
  Hash_SHA256,
  Hash_BLAKE3,
};

enum SyncMethod {
  Sync_SYNC = 1,
  Sync_ARCH,
};

const std::string CONFIG_DIR = ".Dr.Q.config";
const std::string REPOS_CONFIG_FILE = "./data/repos.json";
constexpr int64_t NET_BUFFER_SIZE_BYTES = 4 * 1024 * 1024;  // 8MB
constexpr int64_t CALC_BUFFER_SIZE_BYTES = 64 * 1024;       // 64KB

constexpr int64_t MAX_GRPC_MSG_SIZE = 2 * 64 * 1024 * 1024 * 8;  // 128MB
constexpr double TRACING_SAMPLER_PROBALITITY = 0.01;             // 1 Percent

struct UploadContext {
  std::ifstream file;
  size_t remaining;
};

struct FileAttr {
  std::string path;
  std::string hash;
  std::string enc_hash;
  int64_t size;
  int32_t partition_num;
  int64_t update_time;
  std::string user;
  std::string group;

  std::string ToString() {
    std::string content;
    content.append("path: ");
    content.append(path);
    content.append(", ");

    content.append("hash: ");
    content.append(hash);
    content.append(", ");

    content.append("enc_hash: ");
    content.append(enc_hash);
    content.append(", ");

    content.append("size: ");
    content.append(std::to_string(size));
    content.append(", ");

    content.append("partition_num: ");
    content.append(std::to_string(partition_num));

    content.append("update_time: ");
    content.append(std::to_string(update_time));

    content.append("user: ");
    content.append(user);

    content.append("group: ");
    content.append(group);
    return content;
  }
};

}  // namespace common
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_COMMON_DEFS_H
