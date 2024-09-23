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
#include <unordered_set>
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
    return GenFileDir(path) + "/" + Util::ToHexStr(Util::MurmurHash64A(path));
  }

  bool ValidateCachedStatusFile(const std::string& path) {
    const auto& file_path = GenFileName(path);
    if (!std::filesystem::exists(file_path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }
    proto::ScanStatus scan_status;
    std::unordered_set<std::string> scanned_dirs;
    LoadCachedScanStatus(path, &scan_status, &scanned_dirs);
    for (const auto& file : scan_status.scanned_files()) {
      if (!std::filesystem::exists(file.path())) {
        LOG(ERROR) << "file " << file.path() << " not exists";
        continue;
      }

      if (!std::filesystem::is_regular_file(file.path()) &&
          !std::filesystem::is_symlink(file.path())) {
        LOG(ERROR) << file.path() << " not regular file";
        continue;
      }
    }

    for (const auto& dir : scan_status.scanned_dirs()) {
      if (!std::filesystem::exists(dir)) {
        LOG(ERROR) << "dir " << dir << " not exists";
        continue;
      }
      if (!std::filesystem::is_directory(dir)) {
        LOG(ERROR) << dir << " not directory";
        continue;
      }
    }
    return true;
  }

  void LoadCachedScanStatus(const std::string& path,
                            proto::ScanStatus* scan_status,
                            std::unordered_set<std::string>* scanned_dirs) {
    const std::string& cached_statuspath = GenFileName(path);
    if (!std::filesystem::exists(cached_statuspath)) {
      return;
    }

    std::string content, decompressed_content;
    if (Util::LoadSmallFile(cached_statuspath, &content)) {
      LOG(INFO) << "Load cached scan_status: " << cached_statuspath;
      Util::LZMADecompress(content, &decompressed_content);
      scan_status->ParseFromString(decompressed_content);
      Print(*scan_status);

      for (const auto& dir : scan_status->scanned_dirs()) {
        scanned_dirs->emplace(dir);
      }
    }
  }

  void MarkDirStatus(proto::ScanStatus* scan_status,
                     const std::unordered_set<std::string>& scanned_dirs) {
    scan_status->mutable_scanned_dirs()->Clear();
    scan_status->mutable_scanned_dirs()->Reserve(scanned_dirs.size());
    for (const auto& d : scanned_dirs) {
      *scan_status->mutable_scanned_dirs()->Add() = d;
    }
  }

  void DumpTask(const bool* const stop, const std::string& path,
                proto::ScanStatus* scan_status,
                std::unordered_set<std::string>* scanned_dirs) {
    absl::Mutex mtx;
    while (!*stop) {
      absl::MutexLock lock(&mtx);
      mtx.AwaitWithTimeout(absl::Condition(stop), absl::Seconds(120));
      Dump(GenFileName(path), scan_status, *scanned_dirs);
    }
    LOG(INFO) << "DumpTask Exists";
  }

  bool Dump(const std::string& path, proto::ScanStatus* scan_status,
            const std::unordered_set<std::string>& scanned_dirs) {
    Util::TruncateFile(path);
    std::string content, compressed_content;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      MarkDirStatus(scan_status, scanned_dirs);
      if (scan_status->uuid().empty()) {
        scan_status->set_uuid(Util::UUID());
      }
      scan_status->SerializeToString(&content);
    }
    Util::LZMACompress(content, &compressed_content);
    return Util::WriteToFile(path, compressed_content, true);
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

  bool AddFileItem(const std::string& path, const proto::FileType type,
                   proto::ScanStatus* scan_status) {
    proto::FileItem* file_item = scan_status->mutable_scanned_files()->Add();
    if (!file_item) {
      return false;
    }

    file_item->set_path(path);
    int64_t create_time = 0, update_time = 0, size = 0;
    if (!Util::FileInfo(path, &create_time, &update_time, &size)) {
      return false;
    }
    file_item->set_create_time(create_time);
    file_item->set_update_time(update_time);
    file_item->set_size(size);
    file_item->set_file_type(type);
    return true;
  }

  bool AddFileItemWithLock(const std::string& path, const proto::FileType type,
                           proto::ScanStatus* scan_status) {
    proto::FileItem* file_item = nullptr;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      file_item = scan_status->mutable_scanned_files()->Add();
    }
    if (!file_item) {
      return false;
    }

    file_item->set_path(path);
    int64_t create_time = 0, update_time = 0, size = 0;
    if (!Util::FileInfo(path, &create_time, &update_time, &size)) {
      return false;
    }

    file_item->set_create_time(create_time);
    file_item->set_update_time(update_time);
    file_item->set_size(size);
    file_item->set_file_type(type);
    return true;
  }

  bool Scan(const std::string& path, proto::ScanStatus* scan_status,
            std::unordered_set<std::string>* scanned_dirs,
            bool disable_scan_cache) {
    if (!std::filesystem::exists(path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }

    if (!std::filesystem::is_directory(path)) {
      LOG(ERROR) << path << " not directory";
      return false;
    }

    if (scanning_) {
      LOG(ERROR) << "Another scan is running ...";
      return false;
    }

    if (!SetScanning()) {
      LOG(ERROR) << "Set scanning status error";
      return false;
    }

    if (!disable_scan_cache) {
      LoadCachedScanStatus(path, scan_status, scanned_dirs);
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(&ScanManager::DumpTask, this, &stop_dump_task,
                               path, scan_status, scanned_dirs);
    ThreadPool::Instance()->Post(dump_task);

    auto ret = Scan(path, scan_status, scanned_dirs);
    stop_dump_task = true;

    if (ret) {
      ret = Dump(GenFileName(path), scan_status, *scanned_dirs);
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    scanning_ = false;
    return ret;
  }

  bool Scan(const std::string& path, proto::ScanStatus* scan_status,
            std::unordered_set<std::string>* scanned_dirs) {
    if (stop_.load()) {
      return false;
    }
    {
      auto it = scan_status->ignored_dirs().find(path);
      if (it != scan_status->ignored_dirs().end()) {
        return true;
      }
    }

    auto it = scanned_dirs->find(path);
    if (it != scanned_dirs->end()) {
      return true;
    }

    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink() || !entry.is_directory()) {
          continue;
        }

        if (!Scan(entry.path().string(), scan_status, scanned_dirs)) {
          return false;
        }
      }

      int symlink_num = 0;
      int file_num = 0;
      int dir_num = 0;

      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (stop_.load()) {
          return false;
        }

        if (entry.is_symlink()) {
          AddFileItem(entry.path().string(), proto::FileType::Symlink,
                      scan_status);
          ++symlink_num;
        } else if (entry.is_regular_file()) {
          AddFileItem(entry.path().string(), proto::FileType::Regular,
                      scan_status);
          ++file_num;
        } else if (entry.is_directory()) {
          ++dir_num;
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }

      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        scan_status->set_symlink_num(scan_status->symlink_num() + symlink_num);
        scan_status->set_file_num(scan_status->file_num() + file_num);
        scan_status->set_dir_num(scan_status->dir_num() + dir_num);
      }

      scanned_dirs->insert(path);
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }
    return true;
  }

  bool ParallelScan(const std::string& path, proto::ScanStatus* scan_status,
                    std::unordered_set<std::string>* scanned_dirs,
                    bool disable_scan_cache) {
    if (!std::filesystem::exists(path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }

    if (!std::filesystem::is_directory(path)) {
      LOG(ERROR) << path << " not directory";
      return false;
    }

    if (scanning_.load()) {
      LOG(ERROR) << "Another scan is running ...";
      return false;
    }

    if (!SetScanning()) {
      LOG(ERROR) << "Set scanning status error";
      return false;
    }

    if (!disable_scan_cache) {
      LoadCachedScanStatus(path, scan_status, scanned_dirs);
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(&ScanManager::DumpTask, this, &stop_dump_task,
                               path, scan_status, scanned_dirs);
    ThreadPool::Instance()->Post(dump_task);

    std::packaged_task<bool()> task(std::bind(
        static_cast<bool (ScanManager::*)(
            const std::string&, proto::ScanStatus*,
            std::unordered_set<std::string>*)>(&ScanManager::ParallelScan),
        this, path, scan_status, scanned_dirs));

    auto task_future = task.get_future();
    ++current_threads;
    ThreadPool::Instance()->Post(task);

    auto ret = task_future.get();
    if (ret) {
      ret = Dump(GenFileName(path), scan_status, *scanned_dirs);
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }

    scanning_.store(false);
    return ret;
  }

  bool ParallelScan(const std::string& path, proto::ScanStatus* scan_status,
                    std::unordered_set<std::string>* scanned_dirs) {
    if (stop_.load()) {
      return false;
    }

    {
      auto it = scan_status->ignored_dirs().find(path);
      if (it != scan_status->ignored_dirs().end()) {
        return true;
      }
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = scanned_dirs->find(path);
      if (it != scanned_dirs->end()) {
        return true;
      }
    }

    // LOG(INFO) << "Now scan " << path;
    bool complete = true;
    for (auto it = std::filesystem::directory_iterator(path);
         it != std::filesystem::directory_iterator();) {
      if (stop_.load()) {
        return false;
      }

      if (it->is_symlink() || !it->is_directory()) {
        ++it;
        continue;
      }

      auto cur_thread_next_dir = it->path().string();
      ++it;

      std::vector<std::future<bool>> rets;
      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        while (current_threads <= max_threads &&
               it != std::filesystem::directory_iterator()) {
          if (it->is_symlink() || !it->is_directory()) {
            ++it;
            continue;
          }
          std::packaged_task<bool()> task(
              std::bind(static_cast<bool (ScanManager::*)(
                            const std::string&, proto::ScanStatus*,
                            std::unordered_set<std::string>*)>(
                            &ScanManager::ParallelScan),
                        this, it->path().string(), scan_status, scanned_dirs));
          rets.emplace_back(task.get_future());
          ThreadPool::Instance()->Post(task);
          ++current_threads;
          ++it;
        }
      }

      for (auto& f : rets) {
        if (f.get() == false) {
          complete = false;
        }
        absl::base_internal::SpinLockHolder locker(&lock_);
        --current_threads;
      }

      if (!ParallelScan(cur_thread_next_dir, scan_status, scanned_dirs)) {
        complete = false;
      }
    }

    if (!complete) {
      return false;
    }

    int symlink_num = 0;
    int file_num = 0;
    int dir_num = 0;
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          ++symlink_num;
          if (!AddFileItemWithLock(entry.path().string(),
                                   proto::FileType::Symlink, scan_status)) {
            return false;
          }
        } else if (entry.is_regular_file()) {
          ++file_num;
          if (!AddFileItemWithLock(entry.path().string(),
                                   proto::FileType::Regular, scan_status)) {
            return false;
          }
        } else if (entry.is_directory()) {
          ++dir_num;
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }

    if (complete) {
      absl::base_internal::SpinLockHolder locker(&lock_);
      scan_status->set_symlink_num(scan_status->symlink_num() + symlink_num);
      scan_status->set_file_num(scan_status->file_num() + file_num);
      scan_status->set_dir_num(scan_status->dir_num() + dir_num);
      scanned_dirs->insert(path);
    }
    return complete;
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
