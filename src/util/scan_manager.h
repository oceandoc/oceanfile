/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class ScanContext {
 public:
  ScanContext()
      : status(nullptr),
        hash_method(common::HashMethod::Hash_NONE),
        sync_method(common::SyncMethod::Sync_SYNC),
        disable_scan_cache(false) {
    removed_files.reserve(10000);
  }
  std::string src;
  std::string dst;
  proto::ScanStatus* status;
  common::HashMethod hash_method;
  common::SyncMethod sync_method;
  bool disable_scan_cache;
  std::vector<std::string> removed_files;
  std::unordered_set<std::string> ignored_dirs;
  std::vector<std::string> copy_failed_files;
};

class ScanManager {
 private:
  friend class folly::Singleton<ScanManager>;
  ScanManager() = default;

 public:
  static std::shared_ptr<ScanManager> Instance();

  bool Init() { return true; }

  std::string GenFileDir(const std::string& path) {
    return path + "/" + common::CONFIG_DIR;
  }

  std::string GenFileName(const std::string& path) {
    return GenFileDir(path) + "/" + Util::SHA256(path);
  }

  bool LoadCachedScanStatus(const std::string& path, ScanContext* ctx) {
    const std::string& cached_status_path = GenFileName(path);
    if (!Util::Exists(cached_status_path)) {
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

      ctx->status->mutable_ignored_dirs()->clear();
      for (const auto& dir : ctx->ignored_dirs) {
        ctx->status->mutable_ignored_dirs()->insert({dir, true});
      }

      LOG(INFO) << "Loaed cached status: " << Print(*ctx->status);
      return true;
    }
    return false;
  }

  proto::ErrCode Dump(const std::string& path, ScanContext* ctx) {
    auto ret = proto::ErrCode::Success;
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
      if (!ctx->status->SerializeToString(&content)) {
        LOG(ERROR) << "Serialize error";
        return proto::ErrCode::Serialize_error;
      }
    }
    Util::LZMACompress(content, &compressed_content);
    if (Util::WriteToFile(path, compressed_content, false) ==
            proto::ErrCode::Success ||
        Util::WriteToFile(path, compressed_content, false) ==
            proto::ErrCode::Success) {
      LOG(INFO) << "Dump success: " << Print(*ctx->status);
      return proto::ErrCode::Success;
    }
    LOG(ERROR) << "Dump failed";
    if (cp_ret) {
      Util::CopyFile(path + ".tmp", path);
    }
    return ret;
  }

  void DumpTask(bool* stop_dump_task, ScanContext* ctx) {
    while (!(*stop_dump_task)) {
      std::unique_lock<std::mutex> lock(mu_);
      if (cond_var_.wait_for(lock, std::chrono::minutes(2),
                             [stop_dump_task] { return *stop_dump_task; })) {
        break;
      }
      Dump(GenFileName(ctx->src), ctx);
    }
    LOG(INFO) << "DumpTask Exists";
  }

  bool SetScanning() {
    bool expected = false;
    if (!scanning_.compare_exchange_strong(expected, true)) {
      return false;
    }
    stop_ = false;
    current_threads = 0;
    return true;
  }

  void Stop() {
    stop_.store(true);
    while (scanning_.load()) {
      Util::Sleep(1000);
    }
  }

  std::string Print(const proto::ScanStatus& status) {
    int64_t file_num = 0;
    int64_t symlink_file_num = 0;
    for (const auto& p : status.scanned_files()) {
      if (p.second.file_type() == proto::FileType::Regular) {
        ++file_num;
      } else if (p.second.file_type() == proto::FileType::Symlink) {
        ++symlink_file_num;
      }
    }
    std::stringstream sstream;
    sstream << "scanned_dirs num: " << status.scanned_dirs().size()
            << ", scanned_files num: " << status.scanned_files().size()
            << ", ignored_dirs num: " << status.ignored_dirs().size()
            << ", file_num: " << file_num
            << ", symlink_file_num: " << symlink_file_num;
    return sstream.str();
  }

  proto::ErrCode AddDirItem(const std::string& path, ScanContext* ctx) {
    proto::DirItem dir_item;
    std::string relative_path;
    Util::Relative(path, ctx->src, &relative_path);
    dir_item.set_path(relative_path);
    LOG(INFO) << relative_path;
    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      return proto::ErrCode::File_permission_or_not_exists;
    }

    dir_item.set_update_time(update_time);
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      ctx->status->mutable_scanned_dirs()->insert({relative_path, dir_item});
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode CalcHash(const std::string& path, ScanContext* ctx,
                          proto::FileItem* file_item) {
    if (ctx->hash_method == common::HashMethod::Hash_SHA256) {
      if (!Util::FileSHA256(path, file_item->mutable_sha256())) {
        return proto::ErrCode::File_sha256_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_MD5) {
      if (!Util::FileMD5(path, file_item->mutable_sha256())) {
        return proto::ErrCode::File_sha256_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_CRC32) {
      if (!Util::FileMD5(path, file_item->mutable_sha256())) {
        return proto::ErrCode::File_sha256_calc_error;
      }
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode AddFileItem(const std::string& path,
                             const proto::FileType type, ScanContext* ctx) {
    proto::FileItem file_item;
    std::string relative_path;
    Util::Relative(path, ctx->src, &relative_path);
    file_item.set_path(relative_path);
    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      return proto::ErrCode::File_permission_or_not_exists;
    }

    file_item.set_update_time(update_time);
    file_item.set_size(size);
    file_item.set_file_type(type);
    std::string hash;
    auto ret = CalcHash(path, ctx, &file_item);
    if (ret != proto::ErrCode::Success) {
      return ret;
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      ctx->status->mutable_scanned_files()->insert({relative_path, file_item});
      auto parent_path = Util::ParentPath(relative_path);
      auto it = ctx->status->mutable_scanned_dirs()->find(parent_path);
      if (it == ctx->status->mutable_scanned_dirs()->end()) {
        LOG(INFO) << parent_path;
      }
      // it->second.mutable_files()->insert({relative_path, update_time});
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode RemoveDir(ScanContext* ctx, const std::string& dir) {
    ctx->removed_files.push_back(dir);
    LOG(INFO) << dir;
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = ctx->status->mutable_scanned_dirs()->find(dir);
    for (const auto& p : it->second.files()) {
      ctx->status->mutable_scanned_files()->erase(p.first);
      ctx->removed_files.push_back(p.first);
    }
    ctx->status->mutable_scanned_dirs()->erase(dir);
    return proto::ErrCode::Success;
  }

  proto::ErrCode RemoveFile(ScanContext* ctx, const std::string& dir,
                            const std::set<std::string>& files) {
    std::string relative_path;
    Util::Relative(dir, ctx->src, &relative_path);
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = ctx->status->scanned_dirs().find(relative_path);
    if (it == ctx->status->scanned_dirs().end()) {
      LOG(ERROR) << "This should never happen: " << dir;
      return proto::ErrCode::Fail;
    }

    auto dir_item = it->second;
    for (auto file_it = dir_item.mutable_files()->begin();
         file_it != dir_item.mutable_files()->end();) {
      if (files.find(file_it->first) == files.end()) {
        ctx->status->mutable_scanned_files()->erase(file_it->first);
        ctx->removed_files.push_back(file_it->first);
        file_it = dir_item.mutable_files()->erase(file_it);
      } else {
        ++file_it;
      }
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelScan(ScanContext* ctx) {
    if (!Util::Exists(ctx->src)) {
      LOG(ERROR) << ctx->src << " not exists";
      return proto::ErrCode::Path_not_exists;
    }

    if (!std::filesystem::is_directory(ctx->src)) {
      LOG(ERROR) << ctx->src << " not directory";
      return proto::ErrCode::Path_not_dir;
    }

    if (scanning_.load()) {
      LOG(ERROR) << "Another scan is running ...";
      return proto::ErrCode::Scan_busy;
    }

    if (!SetScanning()) {
      LOG(ERROR) << "Set scanning status error";
      return proto::ErrCode::Scan_set_running_error;
    }

    bool loaded_cached_status = false;
    if (!ctx->disable_scan_cache) {
      loaded_cached_status = LoadCachedScanStatus(ctx->src, ctx);
    }

    bool stop_dump_task = false;
    auto dump_task =
        std::bind(&ScanManager::DumpTask, this, &stop_dump_task, ctx);
    ThreadPool::Instance()->Post(dump_task);

    proto::ErrCode ret = proto::ErrCode::Success;
    std::vector<std::string> dirs;
    if (loaded_cached_status) {
      LOG(INFO) << "Now scan with cache";
      std::vector<std::future<proto::ErrCode>> rets;
      dirs.reserve(ctx->status->scanned_dirs().size());
      for (const auto& p : ctx->status->scanned_dirs()) {
        if (p.first.empty()) {
          dirs.push_back(ctx->src);
        } else {
          dirs.push_back(ctx->src + "/" + p.first);
        }
      }

      for (int32_t i = 0; i < max_threads; ++i) {
        std::packaged_task<proto::ErrCode()> task(
            std::bind(static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
                          const int32_t thread_no, ScanContext*,
                          const std::vector<std::string>&)>(
                          &ScanManager::ParallelScanWithCache),
                      this, i, ctx, dirs));
        rets.emplace_back(task.get_future());
        ThreadPool::Instance()->Post(task);
      }

      for (auto& f : rets) {
        auto f_ret = f.get();
        if (f_ret != proto::ErrCode::Success) {
          ret = f_ret;
        }
      }
    } else {
      LOG(INFO) << "Now full scan";
      std::packaged_task<proto::ErrCode()> task(
          std::bind(static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
                        const std::string&, ScanContext*)>(
                        &ScanManager::ParallelFullScan),
                    this, ctx->src, ctx));

      auto task_future = task.get_future();
      ++current_threads;
      ThreadPool::Instance()->Post(task);
      ret = task_future.get();
    }

    LOG(INFO) << 1;
    stop_dump_task = true;
    cond_var_.notify_all();
    current_threads = 0;
    if (ret != proto::ErrCode::Success) {
      // below sleep can avoid stop_dump_task destruct too fast caused coredump,
      Util::Sleep(10);
      scanning_.store(false);
      return ret;
    }

    ret = Dump(GenFileName(ctx->src), ctx);
    scanning_.store(false);
    if (ret == proto::ErrCode::Success) {
      ctx->status->set_complete_time(Util::CurrentTimeMillis());
    }
    return ret;
  }

  proto::ErrCode ParallelScanWithCache(const int32_t thread_no,
                                       ScanContext* ctx,
                                       const std::vector<std::string>& dirs) {
    for (size_t i = thread_no; i < dirs.size(); i += max_threads) {
      if (stop_.load()) {
        return proto::ErrCode::Scan_interrupted;
      }

      auto ret = proto::ErrCode::Success;
      const auto& dir = (dirs)[i];
      if (!Util::Exists(dir)) {
        RemoveDir(ctx, dir);
        continue;
      }

      auto update_time = Util::UpdateTime(dir);
      {
        std::string relative_path;
        Util::Relative(dir, ctx->src, &relative_path);
        absl::base_internal::SpinLockHolder locker(&lock_);
        auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
        if (it->second.update_time() == update_time) {
          continue;
        }
        it->second.set_update_time(update_time);
      }

      ret = Scan(dir, ctx);
      if (ret != proto::ErrCode::Success) {
        return ret;
      }
    }

    return proto::ErrCode::Success;
  }

  proto::ErrCode Scan(const std::string& path, ScanContext* ctx) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    std::set<std::string> files;
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        LOG(INFO) << entry.path().string();
        files.insert(entry.path().string());
        auto ret = proto::ErrCode::Success;
        const auto& entry_path = entry.path().string();
        if (entry.is_symlink()) {
          ret = AddFileItem(entry_path, proto::FileType::Symlink, ctx);
        } else if (entry.is_regular_file()) {
          ret = AddFileItem(entry_path, proto::FileType::Regular, ctx);
        } else if (entry.is_directory()) {
          {
            std::string relative_path;
            Util::Relative(entry_path, ctx->src, &relative_path);
            absl::base_internal::SpinLockHolder locker(&lock_);
            auto it = ctx->status->scanned_dirs().find(relative_path);
            if (it != ctx->status->scanned_dirs().end()) {
              continue;
            }
          }
          ret = AddDirItem(entry.path().string(), ctx);
          ret = Scan(entry.path().string(), ctx);
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
        if (ret != proto::ErrCode::Success) {
          return ret;
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return proto::ErrCode::Fail;
    }

    // if (RemoveFile(ctx, path, files) != proto::ErrCode::Success) {
    if (!RemoveFile(ctx, path, files)) {
      return proto::ErrCode::Fail;
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelFullScan(const std::string& path, ScanContext* ctx) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    auto it = ctx->status->ignored_dirs().find(path);
    if (it != ctx->status->ignored_dirs().end()) {
      return proto::ErrCode::Success;
    }

    proto::ErrCode all_ret = proto::ErrCode::Success;
    for (auto it = std::filesystem::directory_iterator(path);
         it != std::filesystem::directory_iterator();) {
      if (stop_.load()) {
        return proto::ErrCode::Scan_interrupted;
      }

      if (it->is_symlink() || !it->is_directory()) {
        ++it;
        continue;
      }

      auto cur_thread_next_dir = it->path().string();
      ++it;
      std::vector<std::future<proto::ErrCode>> rets;
      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        while (current_threads <= max_threads &&
               it != std::filesystem::directory_iterator()) {
          if (it->is_symlink() || !it->is_directory()) {
            ++it;
            continue;
          }
          std::packaged_task<proto::ErrCode()> task(
              std::bind(static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
                            const std::string&, ScanContext*)>(
                            &ScanManager::ParallelFullScan),
                        this, it->path().string(), ctx));
          rets.emplace_back(task.get_future());
          ThreadPool::Instance()->Post(task);
          ++current_threads;
          ++it;
        }
      }

      for (auto& f : rets) {
        if (f.get() != proto::ErrCode::Success) {
          all_ret = f.get();
        }
        absl::base_internal::SpinLockHolder locker(&lock_);
        --current_threads;
      }

      auto ret = ParallelFullScan(cur_thread_next_dir, ctx);
      if (ret != proto::ErrCode::Success) {
        all_ret = ret;
      }
    }

    if (all_ret != proto::ErrCode::Success) {
      return all_ret;
    }

    AddDirItem(path, ctx);

    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        auto ret = proto::ErrCode::Success;
        const auto& entry_path = entry.path().string();
        if (entry.is_symlink()) {
          ret = AddFileItem(entry_path, proto::FileType::Symlink, ctx);
        } else if (entry.is_regular_file()) {
          ret = AddFileItem(entry_path, proto::FileType::Regular, ctx);
        } else if (entry.is_directory()) {
          ret = AddDirItem(entry_path, ctx);
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
        if (ret != proto::ErrCode::Success) {
          return ret;
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return proto::ErrCode::Fail;
    }

    return proto::ErrCode::Success;
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> scanning_ = false;
  std::atomic<bool> stop_ = false;
  int current_threads = 0;
  const int max_threads = 1;
  std::mutex mu_;
  std::condition_variable cond_var_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
