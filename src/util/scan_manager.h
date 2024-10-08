/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/blocking_queue.h"
#include "src/common/defs.h"
#include "src/common/error.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

// TODO(xieyz) add full unit_test
// TODO(xieyz) dump to disk only file or dir change count over 10 thousand
namespace oceandoc {
namespace util {

class ScanContext {
 public:
  ScanContext()
      : status(nullptr),
        hash_method(common::HashMethod::Hash_NONE),
        sync_method(common::SyncMethod::Sync_SYNC),
        disable_scan_cache(false),
        skip_scan(false) {
    copy_failed_files.reserve(10000);
  }
  std::string src;
  std::string dst;
  proto::ScanStatus* status;
  common::HashMethod hash_method;
  common::SyncMethod sync_method;
  bool disable_scan_cache;
  std::atomic<int32_t> scanned_dir_num = 0;
  std::atomic<int32_t> skip_dir_num = 0;
  bool skip_scan;

  std::set<std::string> removed_files;           // full path
  std::set<std::string> added_files;             // full path
  std::unordered_set<std::string> ignored_dirs;  // relative to src
  std::vector<std::string> copy_failed_files;    // full path
  int32_t err_code = Err_Success;
};

class ScanManager {
 private:
  friend class folly::Singleton<ScanManager>;
  ScanManager() = default;

 public:
  static std::shared_ptr<ScanManager> Instance();

  bool Init() { return true; }

  std::string GenFileName(const std::string& path) {
    return path + "/" + common::CONFIG_DIR + "/" + Util::SHA256(path);
  }

  bool LoadCachedScanStatus(ScanContext* ctx) {
    const std::string& cached_status_path = GenFileName(ctx->src);
    if (!Util::Exists(cached_status_path)) {
      LOG(INFO) << cached_status_path << " not exists";
      return false;
    }

    std::string content, decompressed_content;
    if (Util::LoadSmallFile(cached_status_path, &content)) {
      if (!Util::LZMADecompress(content, &decompressed_content)) {
        LOG(ERROR) << "Decomppress error: " << cached_status_path;
        return false;
      }
      if (!ctx->status->ParseFromString(decompressed_content)) {
        LOG(ERROR) << "Parse error: " << cached_status_path;
        return false;
      }

      for (const auto& dir : ctx->ignored_dirs) {
        ctx->status->mutable_ignored_dirs()->insert({dir, true});
      }

      LOG(INFO) << "Load cache status success: " << Print(*ctx);
      return true;
    }
    LOG(ERROR) << "Load cache status error";
    return false;
  }

  bool ValidateScanStatus(ScanContext* ctx) {
    const auto& path = ctx->src;
    if (!Util::Exists(path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }

    if (!LoadCachedScanStatus(ctx)) {
      return false;
    }
    const auto& status = *ctx->status;

    if (status.path() != path) {
      LOG(ERROR) << "Does " << path << " moved from " << status.path();
      return false;
    }

    int64_t file_num = 0;
    int64_t symlink_num = 0;
    for (const auto& p : status.scanned_dirs()) {
      if (p.first != p.second.path()) {
        LOG(ERROR) << "This should never happend";
        return false;
      }

      if (Util::IsAbsolute(p.first)) {
        LOG(ERROR) << "Path should be relative: " << p.first;
        return false;
      }

      const auto& file_items = p.second.files();
      for (const auto& file : file_items) {
        if (file.second.file_type() == proto::FileType::Regular) {
          ++file_num;
        } else {
          ++symlink_num;
        }
      }
    }

    if (file_num != status.file_num() || symlink_num != status.symlink_num()) {
      LOG(ERROR) << "file num or symlink num inconsistent: status file num: "
                 << status.file_num() << ", statistic file num: " << file_num
                 << ", status symlink num: " << status.symlink_num()
                 << ", statistic symlink num: " << symlink_num;
      return false;
    }

    LOG(INFO) << "Validate success, " << Print(*ctx);
    return true;
  }

  void SumStatus(ScanContext* ctx) {
    int64_t file_num = 0;
    int64_t symlink_num = 0;
    for (const auto& p : ctx->status->scanned_dirs()) {
      for (const auto& t : p.second.files()) {
        if (t.second.file_type() == proto::FileType::Regular) {
          ++file_num;
        } else {
          ++symlink_num;
        }
      }
    }
    ctx->status->set_file_num(file_num);
    ctx->status->set_symlink_num(symlink_num);
  }

  int32_t Dump(ScanContext* ctx) {
    std::string path = GenFileName(ctx->src);
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (ctx->status->uuid().empty()) {
        ctx->status->set_uuid(Util::UUID());
      }
    }

    bool cp_ret = true;
    if (Util::Exists(path)) {
      cp_ret = Util::CopyFile(path, path + ".tmp");
    }

    std::string content, compressed_content;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      SumStatus(ctx);
      if (!ctx->status->SerializeToString(&content)) {
        LOG(ERROR) << "Serialize error";
        return Err_Serialize_error;
      }
    }
    Util::LZMACompress(content, &compressed_content);
    if (Util::WriteToFile(path, compressed_content, false) == Err_Success ||
        Util::WriteToFile(path, compressed_content, false) == Err_Success) {
      LOG(INFO) << "Dump success: " << Print(*ctx);
      return Err_Success;
    }
    LOG(ERROR) << "Dump failed";
    if (cp_ret) {
      Util::CopyFile(path + ".tmp", path);
    }
    return Err_Scan_dump_error;
  }

  void DumpTask(bool* stop_dump_task, ScanContext* ctx) {
    int64_t last_time = Util::CurrentTimeMillis();
    while (!(*stop_dump_task)) {
      std::unique_lock<std::mutex> lock(mu_);
      if (cond_var_.wait_for(lock, std::chrono::seconds(10),
                             [stop_dump_task] { return *stop_dump_task; })) {
        break;
      }
      int64_t cur_time = Util::CurrentTimeMillis();
      if (cur_time - last_time > 10 * 60 * 1000) {
        Dump(ctx);
        last_time = Util::CurrentTimeMillis();
      }
      LOG(INFO) << "Resident memory usage: " << Util::MemUsage()
                << "MB, scanned dir num: " << ctx->scanned_dir_num
                << ", skip dir num: " << ctx->skip_dir_num;
    }
    LOG(INFO) << "DumpTask Exists";
  }

  bool SetScanning() {
    bool expected = false;
    if (!scanning_.compare_exchange_strong(expected, true)) {
      return false;
    }
    running_mark_ = 0;
    stop_ = false;
    running_threads_ = 0;
    stop_dump_task_ = false;
    dir_queue_.Clear();
    return true;
  }

  void Stop() {
    stop_.store(true);
    while (scanning_.load()) {
      Util::Sleep(1000);
    }
  }

  std::string Print(const ScanContext& ctx) {
    std::stringstream sstream;
    absl::base_internal::SpinLockHolder locker(&lock_);
    sstream << "scanned_dirs num: " << ctx.status->scanned_dirs().size()
            << ", file_num: " << ctx.status->file_num()
            << ", symlink_file_num: " << ctx.status->symlink_num()
            << ", ignored_dirs num: " << ctx.status->ignored_dirs().size()
            << ", added_files num: " << ctx.added_files.size()
            << ", removed_files num: " << ctx.removed_files.size();
    return sstream.str();
  }

  int32_t AddDirItem(const std::string& path, ScanContext* ctx,
                     const std::string& relative_path) {
    proto::DirItem dir_item;
    dir_item.set_path(relative_path);

    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      LOG(ERROR) << "FileInfo error: " << path;
      return Err_File_permission_or_not_exists;
    }
    dir_item.set_update_time(update_time);

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
      if (it != ctx->status->mutable_scanned_dirs()->end()) {
        it->second.Swap(&dir_item);
      } else {
        (*ctx->status->mutable_scanned_dirs())[relative_path] = dir_item;
        ctx->added_files.insert(path);
      }
    }
    return Err_Success;
  }

  int32_t CalcHash(const std::string& path, ScanContext* ctx,
                   proto::FileItem* file_item) {
    if (ctx->hash_method == common::HashMethod::Hash_SHA256) {
      if (!Util::FileSHA256(path, file_item->mutable_sha256())) {
        return Err_File_hash_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_MD5) {
      if (!Util::FileMD5(path, file_item->mutable_sha256())) {
        return Err_File_hash_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_CRC32) {
      if (!Util::FileMD5(path, file_item->mutable_sha256())) {
        return Err_File_hash_calc_error;
      }
    }
    return Err_Success;
  }

  int32_t AddFileItem(const std::string& filename, const proto::FileType type,
                      ScanContext* ctx,
                      const std::string& parent_relative_path) {
    const auto& path = ctx->src + "/" + parent_relative_path + "/" + filename;
    proto::FileItem file_item;
    file_item.set_filename(filename);
    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      LOG(ERROR) << "FileInfo error: " << path;
      return Err_File_permission_or_not_exists;
    }

    file_item.set_update_time(update_time);
    file_item.set_size(size);
    file_item.set_file_type(type);

    auto ret = CalcHash(path, ctx, &file_item);
    if (ret != Err_Success) {
      LOG(ERROR) << "CalcHash error: " << path;
      return ret;
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto dir_it =
          ctx->status->mutable_scanned_dirs()->find(parent_relative_path);
      if (dir_it == ctx->status->mutable_scanned_dirs()->end()) {
        LOG(ERROR) << "This should never happen, parent_path: "
                   << parent_relative_path;
        return Err_Fail;
      }
      auto file_it = dir_it->second.mutable_files()->find(filename);
      if (file_it != dir_it->second.mutable_files()->end()) {
        file_it->second.Swap(&file_item);
      } else {
        (*dir_it->second.mutable_files())[filename] = file_item;
        ctx->added_files.emplace(path);
      }
    }
    return Err_Success;
  }

  int32_t RemoveDir(ScanContext* ctx, const std::string& cur_dir,
                    const std::string& relative_path) {
    ctx->removed_files.insert(cur_dir);

    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
    if (it == ctx->status->mutable_scanned_dirs()->end()) {
      return Err_Success;
    }

    for (const auto& p : it->second.files()) {
      ctx->removed_files.emplace(cur_dir + "/" + p.first);
    }

    ctx->status->mutable_scanned_dirs()->erase(relative_path);
    return Err_Success;
  }

  int32_t RemoveFile(ScanContext* ctx, const std::string& cur_dir,
                     const std::set<std::string>& files,
                     const std::string& relative_path) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
    if (it == ctx->status->mutable_scanned_dirs()->end()) {
      LOG(ERROR) << "This should never happen: " << cur_dir;
      return Err_Fail;
    }

    for (auto file_it = it->second.mutable_files()->begin();
         file_it != it->second.mutable_files()->end();) {
      if (files.find(file_it->first) == files.end()) {
        ctx->removed_files.emplace(cur_dir + "/" + file_it->first);
        file_it = it->second.mutable_files()->erase(file_it);
      } else {
        ++file_it;
      }
    }
    return Err_Success;
  }

  int32_t ParallelScan(ScanContext* ctx) {
    if (!Util::Exists(ctx->src)) {
      LOG(ERROR) << ctx->src << " not exists";
      return Err_Path_not_exists;
    }

    if (!std::filesystem::is_directory(ctx->src)) {
      LOG(ERROR) << ctx->src << " not directory";
      return Err_Path_not_dir;
    }

    if (scanning_.load()) {
      LOG(ERROR) << "Another scan is running ...";
      return Err_Scan_busy;
    }

    if (!SetScanning()) {
      LOG(ERROR) << "Set scanning status error";
      return Err_Scan_set_running_error;
    }

    LOG(INFO) << "Now scan " << ctx->src;
    bool load_cache_success = true;
    if (!ctx->disable_scan_cache) {
      load_cache_success = LoadCachedScanStatus(ctx);
    }

    if (load_cache_success && ctx->skip_scan) {
      return Err_Success;
    }

    dir_queue_.PushBack(ctx->src);  // all elements is absolute path
    for (const auto& p : ctx->status->scanned_dirs()) {
      if (p.first.empty()) {
        continue;
      }
      dir_queue_.PushBack(ctx->src + "/" + p.first);
    }

    auto dump_task =
        std::bind(&ScanManager::DumpTask, this, &stop_dump_task_, ctx);
    ThreadPool::Instance()->Post(dump_task);

    for (int32_t i = 0; i < max_threads; ++i) {
      running_mark_.fetch_or(1ULL << i);
      ++running_threads_;
      LOG(INFO) << "Thread " << i << " running";
      auto task = std::bind(&ScanManager::ParallelFullScan, this, i, ctx);
      ThreadPool::Instance()->Post(task);
    }
    LOG(INFO) << "Scan begin, current_threads: " << max_threads;

    while (running_mark_) {
      for (int32_t i = 0; i < max_threads; ++i) {
        if (running_mark_ & (1ULL << i)) {
          continue;
        }
        ++running_threads_;
        running_mark_.fetch_or(1ULL << i);
        LOG(INFO) << "Thread " << i << " running";
        auto task = std::bind(&ScanManager::ParallelFullScan, this, i, ctx);
        ThreadPool::Instance()->Post(task);
      }
      Util::Sleep(1000);
    }

    stop_dump_task_ = true;
    cond_var_.notify_all();

    if (ctx->err_code != Err_Success) {
      LOG(ERROR) << "Scan has error: " << ctx->err_code;
    }

    if (ctx->err_code == Err_Success) {
      ctx->status->set_complete_time(Util::CurrentTimeMillis());
      ctx->status->set_hash_method((int32_t)ctx->hash_method);
    }
    LOG(INFO) << "Scan finished, current_threads: " << running_threads_;
    running_threads_ = 0;
    running_mark_ = 0;

    if (Dump(ctx) != Err_Success) {
      ctx->err_code = Err_Scan_dump_error;
    }

    scanning_.store(false);
    return ctx->err_code;
  }

  void ParallelFullScan(const int32_t thread_no, ScanContext* ctx) {
    static thread_local std::atomic<int32_t> count = 0;
    while (true) {
      if (stop_.load()) {
        ctx->err_code = Err_Scan_interrupted;
        break;
      }

      std::string cur_dir;
      int try_times = 0;
      while (!dir_queue_.PopBack(&cur_dir) && try_times < 5) {
        Util::Sleep(100);
        ++try_times;
      }

      if (try_times >= 5) {
        break;
      }

      std::string relative_path;
      Util::Relative(cur_dir, ctx->src, &relative_path);
      ctx->scanned_dir_num.fetch_add(1);
      auto it = ctx->status->ignored_dirs().find(relative_path);
      if (it != ctx->status->ignored_dirs().end()) {
        continue;
      }

      if (!Util::Exists(cur_dir)) {
        RemoveDir(ctx, cur_dir, relative_path);
        continue;
      }

      auto update_time = Util::UpdateTime(cur_dir);
      if (update_time == -1) {
        LOG(INFO) << "Scan error: " << cur_dir;
        ctx->err_code = Err_File_permission_or_not_exists;
        continue;
      }

      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
        if (it != ctx->status->mutable_scanned_dirs()->end()) {
          if (it->second.update_time() == update_time &&
              ctx->hash_method ==
                  common::HashMethod(ctx->status->hash_method())) {
            ctx->skip_dir_num.fetch_add(1);
            continue;
          }
        }
      }

      auto ret = AddDirItem(cur_dir, ctx, relative_path);
      if (ret != Err_Success) {
        ctx->err_code = ret;
        continue;
      }

      ret = Err_Success;
      try {
        std::set<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
          const auto& filename = entry.path().filename().string();
          files.insert(filename);
          if (entry.is_symlink()) {
            ret |= AddFileItem(filename, proto::FileType::Symlink, ctx,
                               relative_path);
          } else if (entry.is_regular_file()) {
            ret |= AddFileItem(filename, proto::FileType::Regular, ctx,
                               relative_path);
          } else if (entry.is_directory()) {
            dir_queue_.PushBack(entry.path().string());
          } else {
            LOG(ERROR) << "Unknow file type: " << entry.path();
          }
        }

        if (ret != Err_Success) {
          ctx->err_code = ret;
          continue;
        }

        RemoveFile(ctx, cur_dir, files, relative_path);

        {
          absl::base_internal::SpinLockHolder locker(&lock_);
          auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
          if (it != ctx->status->mutable_scanned_dirs()->end()) {
            it->second.set_update_time(update_time);
          }
        }

      } catch (const std::filesystem::filesystem_error& e) {
        LOG(ERROR) << "Scan error: " << cur_dir << ", exception: " << e.what();
      }

      if ((count % 3000) == 0) {
        LOG(INFO) << "Scanning: " << cur_dir << ", thread_no: " << thread_no;
      }
      ++count;
    }
    running_mark_.fetch_and(~(1ULL << thread_no));
    --running_threads_;
    LOG(INFO) << "Thread " << thread_no << " exist";
  }

 private:
  mutable absl::base_internal::SpinLock lock_;

  std::atomic<bool> scanning_ = false;
  std::atomic<bool> stop_ = false;
  std::atomic<uint64_t> running_mark_ = 0;
  std::atomic<int32_t> running_threads_ = 0;
  bool stop_dump_task_ = false;

  const int max_threads = 4;
  std::mutex mu_;
  std::condition_variable cond_var_;
  common::BlockingQueue<std::string> dir_queue_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
