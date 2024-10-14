/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_COMMON_DEFS_H
#define BAZEL_TEMPLATE_COMMON_DEFS_H

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>

#include "src/common/blocking_queue.h"
#include "src/common/error.h"
#include "src/proto/data.pb.h"
#include "src/proto/service.pb.h"

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

class ScanContext {
 public:
  ScanContext(const uint32_t max_threads = 4)
      : status(nullptr),
        hash_method(HashMethod::Hash_NONE),
        sync_method(SyncMethod::Sync_SYNC),
        disable_scan_cache(false),
        skip_scan(false),
        max_threads(max_threads) {}

  void Reset() {
    scanned_dir_num = 0;
    skip_dir_num = 0;
    stop_dump_task = false;
    removed_files.clear();
    added_files.clear();
    err_code = Err_Success;
    running_mark = 0;
    dir_queue.Clear();
  }

  std::string src;
  std::string dst;
  proto::ScanStatus* status;
  HashMethod hash_method;
  SyncMethod sync_method;
  bool disable_scan_cache = false;
  bool skip_scan = false;
  std::unordered_set<std::string> ignored_dirs;  // relative to src

  mutable absl::base_internal::SpinLock lock;
  std::mutex mu;
  std::condition_variable cond_var;
  const int max_threads;

  std::atomic<int32_t> scanned_dir_num = 0;
  std::atomic<int32_t> skip_dir_num = 0;
  bool stop_dump_task = false;
  std::set<std::string> removed_files;  // full path
  std::set<std::string> added_files;    // full path
  int32_t err_code = Err_Success;
  std::atomic<uint64_t> running_mark = 0;
  BlockingQueue<std::string> dir_queue;
};

class SyncContext {
 public:
  SyncContext(const int max_threads = 4) : max_threads(max_threads) {}
  SyncContext(const std::string& remote_addr, const std::string& remote_port,
              const int max_threads = 4)
      : remote_addr(remote_addr),
        remote_port(remote_port),
        max_threads(max_threads) {}

  std::string remote_addr;
  std::string remote_port;
  const int max_threads;
  int64_t partition_size;
  std::string src;
  std::string dst;
  proto::RepoType repo_type;
  HashMethod hash_method = HashMethod::Hash_NONE;
  SyncMethod sync_method = SyncMethod::Sync_SYNC;
  bool disable_scan_cache = false;
  bool skip_scan = false;
  std::unordered_set<std::string> ignored_dirs;  // relative to src

  mutable absl::base_internal::SpinLock lock;
  std::mutex mu;
  std::condition_variable cond_var;

  bool stop_progress_task = false;
  std::atomic<int64_t> syncd_total_cnt = 0;
  std::atomic<int64_t> syncd_success_cnt = 0;
  std::atomic<int64_t> syncd_fail_cnt = 0;
  std::atomic<int64_t> syncd_skipped_cnt = 0;
  std::vector<std::string> copy_failed_files;  // full path
  int32_t err_code = Err_Success;
  ScanContext* scan_ctx = nullptr;
  BlockingQueue<std::string> dir_queue;
  std::atomic<uint64_t> running_mark = 0;

  void Reset() {
    stop_progress_task = false;
    syncd_total_cnt = 0;
    syncd_success_cnt = 0;
    syncd_fail_cnt = 0;
    syncd_skipped_cnt = 0;
    copy_failed_files.clear();
    err_code = Err_Success;
    scan_ctx = nullptr;
    dir_queue.Clear();
    running_mark = 0;
  }
};

class SendContext {
 public:
  std::string repo_uuid;
  std::string src;
  std::string dst;
  proto::FileType type;
  std::string content;
  std::string hash;
  proto::FileOp op;
  std::vector<int32_t> mark;
};

class GCEntry {
 public:
  GCEntry(SendContext* p) : p(p) {}

  ~GCEntry() {
    if (p) {
      delete p;
      p = nullptr;
    }
  }

 private:
  SendContext* p;
};

}  // namespace common
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_COMMON_DEFS_H
