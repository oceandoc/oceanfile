/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "absl/synchronization/mutex.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

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

  void LoadCachedScanStatus(
      const std::string& path, proto::ScanStatus* scan_status,
      std::unordered_map<std::string, int64_t>* scanned_dirs,
      std::unordered_map<std::string, proto::FileItem>* scanned_files) {
    const std::string& cached_status_path = GenFileName(path);
    if (!Util::Exists(cached_status_path)) {
      return;
    }

    std::string content, decompressed_content;
    if (Util::LoadSmallFile(cached_status_path, &content)) {
      LOG(INFO) << "Load cached scan_status success: " << cached_status_path;
      Util::LZMADecompress(content, &decompressed_content);
      scan_status->ParseFromString(decompressed_content);
      Print(*scan_status);

      for (const auto& dir : scan_status->scanned_dirs()) {
        scanned_dirs->insert({dir.path(), dir.update_time()});
      }

      for (const auto& file : scan_status->scanned_files()) {
        scanned_files->insert({file.path(), file});
      }
      scan_status->mutable_scanned_dirs()->Clear();
      scan_status->mutable_scanned_files()->Clear();
    }
  }

  void UpdateScanStatus(
      proto::ScanStatus* scan_status,
      const std::unordered_map<std::string, int64_t>& scanned_dirs,
      const std::unordered_map<std::string, proto::FileItem>& scanned_files) {
    scan_status->mutable_scanned_dirs()->Clear();
    scan_status->mutable_scanned_dirs()->Reserve(scanned_dirs.size());
    scan_status->mutable_scanned_files()->Clear();
    scan_status->mutable_scanned_files()->Reserve(scanned_files.size());
    for (const auto& d : scanned_dirs) {
      auto dir_item = *scan_status->mutable_scanned_dirs()->Add();
      dir_item.set_path(d.first);
      dir_item.set_update_time(d.second);
    }
    for (const auto& f : scanned_files) {
      *scan_status->mutable_scanned_files()->Add() = f.second;
    }
  }

  void DumpTask(
      const bool* const stop, const std::string& path,
      proto::ScanStatus* scan_status,
      std::unordered_map<std::string, int64_t>* scanned_dirs,
      std::unordered_map<std::string, proto::FileItem>* scanned_files) {
    absl::Mutex mtx;
    while (!*stop) {
      absl::MutexLock lock(&mtx);
      mtx.AwaitWithTimeout(absl::Condition(stop), absl::Seconds(120));
      Dump(GenFileName(path), scan_status, *scanned_dirs, *scanned_files);
    }
    LOG(INFO) << "DumpTask Exists";
  }

  // TODO cop to tmp first
  proto::ErrCode Dump(
      const std::string& path, proto::ScanStatus* scan_status,
      const std::unordered_map<std::string, int64_t>& scanned_dirs,
      const std::unordered_map<std::string, proto::FileItem>& scanned_files) {
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      UpdateScanStatus(scan_status, scanned_dirs, scanned_files);
    }
    if (scan_status->uuid().empty()) {
      scan_status->set_uuid(Util::UUID());
    }
    std::string content, compressed_content;
    scan_status->SerializeToString(&content);
    Util::LZMACompress(content, &compressed_content);
    if (!Util::WriteToFile(path, compressed_content, false)) {
      return proto::ErrCode::Scan_dump_error;
    }
    return proto::ErrCode::Success;
  }

  bool SetScanning() {
    bool expected = false;
    if (!scanning_.compare_exchange_strong(expected, true)) {
      return false;
    }
    current_threads = 0;
    return true;
  }

  void Stop() {
    stop_.store(true);
    while (scanning_.load()) {
      Util::Sleep(1000);
    }
  }

  void Print(const proto::ScanStatus& scan_status) {
    LOG(INFO) << "file_num: " << scan_status.file_num()
              << ", dir_num: " << scan_status.dir_num() + 1  // self
              << ", symlink_num: " << scan_status.symlink_num()
              << ", scanned_files num: " << scan_status.scanned_files().size()
              << ", scanned_dirs num: " << scan_status.scanned_dirs().size()
              << ", ignored_dirs num: " << scan_status.ignored_dirs().size();
  }

  proto::ErrCode AddFileItemWithLock(
      const std::string& path, const proto::FileType type,
      std::unordered_map<std::string, proto::FileItem>* scanned_files) {
    proto::FileItem file_item;
    file_item.set_path(path);
    int64_t create_time = 0, update_time = 0, size = 0;
    if (!Util::FileInfo(path, &create_time, &update_time, &size)) {
      return proto::ErrCode::File_permission_or_not_exists;
    }

    file_item.set_update_time(update_time);
    file_item.set_size(size);
    file_item.set_file_type(type);
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      scanned_files->insert({path, file_item});
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelScan(
      const std::string& path, proto::ScanStatus* scan_status,
      std::unordered_map<std::string, int64_t>* scanned_dirs,
      std::unordered_map<std::string, proto::FileItem>* scanned_files,
      const bool calc_hash, const bool disable_scan_cache) {
    if (!Util::Exists(path)) {
      LOG(ERROR) << path << " not exists";
      return proto::ErrCode::Path_not_exists;
    }

    if (!std::filesystem::is_directory(path)) {
      LOG(ERROR) << path << " not directory";
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

    if (!disable_scan_cache) {
      LoadCachedScanStatus(path, scan_status, scanned_dirs, scanned_files);
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(&ScanManager::DumpTask, this, &stop_dump_task,
                               path, scan_status, scanned_dirs, scanned_files);
    ThreadPool::Instance()->Post(dump_task);

    std::packaged_task<proto::ErrCode()> task(std::bind(
        static_cast<proto::ErrCode (ScanManager::*)(
            const std::string&, proto::ScanStatus*,
            std::unordered_map<std::string, int64_t>*,
            std::unordered_map<std::string, proto::FileItem>*,
            const bool calc_hash)>(&ScanManager::ParallelScan),
        this, path, scan_status, scanned_dirs, scanned_files, calc_hash));

    auto task_future = task.get_future();
    ++current_threads;
    ThreadPool::Instance()->Post(task);

    auto ret = task_future.get();
    if (!ret) {
      ret = Dump(GenFileName(path), scan_status, *scanned_dirs, *scanned_files);
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }

    scanning_.store(false);
    return ret;
  }

  proto::ErrCode ParallelScan(
      const std::string& path, proto::ScanStatus* scan_status,
      std::unordered_map<std::string, int64_t>* scanned_dirs,
      std::unordered_map<std::string, proto::FileItem>* scanned_files,
      const bool calc_hash) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    {
      auto it = scan_status->ignored_dirs().find(path);
      if (it != scan_status->ignored_dirs().end()) {
        return proto::ErrCode::Success;
      }
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = scanned_dirs->find(path);
      if (it != scanned_dirs->end()) {
        auto upadte_time = Util::UpdateTime(path);
        if (it->second == upadte_time) {
          return proto::ErrCode::Success;
        }
      }
    }

    // LOG(INFO) << "Now scan " << path;
    bool complete = true;
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
              std::bind(static_cast<proto::ErrCode (ScanManager::*)(
                            const std::string&, proto::ScanStatus*,
                            std::unordered_map<std::string, int64_t>*,
                            std::unordered_map<std::string, proto::FileItem>*,
                            const bool)>(&ScanManager::ParallelScan),
                        this, it->path().string(), scan_status, scanned_dirs,
                        scanned_files, calc_hash));
          rets.emplace_back(task.get_future());
          ThreadPool::Instance()->Post(task);
          ++current_threads;
          ++it;
        }
      }

      for (auto& f : rets) {
        if (!f.get()) {
          complete = false;
        }
        absl::base_internal::SpinLockHolder locker(&lock_);
        --current_threads;
      }

      auto ret = ParallelScan(cur_thread_next_dir, scan_status, scanned_dirs,
                              scanned_files, calc_hash);

      if (!ret) {
        complete = false;
      }
    }

    if (!complete) {
      return proto::ErrCode::Fail;
    }

    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          auto ret = AddFileItemWithLock(
              entry.path().string(), proto::FileType::Symlink, scanned_files);
          if (!ret) {
            return ret;
          }
        } else if (entry.is_regular_file()) {
          auto ret = AddFileItemWithLock(
              entry.path().string(), proto::FileType::Regular, scanned_files);
          if (!ret) {
            return ret;
          }
        } else if (entry.is_directory()) {
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return proto::ErrCode::Fail;
    }

    if (complete) {
      auto update_time = Util::UpdateTime(path);
      absl::base_internal::SpinLockHolder locker(&lock_);
      scanned_dirs->insert({path, update_time});
    }
    return proto::ErrCode::Success;
  }

 public:
 private:
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> scanning_ = false;
  std::atomic<bool> stop_ = false;
  int current_threads = 0;
  const int max_threads = 3;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
