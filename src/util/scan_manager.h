/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H

#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

// TODO
// 1. remark scan complete scan_status by compare update time

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
    return path + "/" + mark_dir_name;
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
    const std::string cached_statuspath = GenFileName(path);
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

  bool Dump(const std::string& path, const proto::ScanStatus& scan_status) {
    Util::TruncateFile(path);
    std::string content, compressed_content;
    scan_status.SerializeToString(&content);
    Util::LZMACompress(content, &compressed_content);
    return Util::WriteToFile(path, compressed_content, true);
  }

  void MarkDirStatus(proto::ScanStatus* scan_status,
                     const std::unordered_set<std::string>& scanned_dirs) {
    scan_status->mutable_scanned_dirs()->Clear();
    scan_status->mutable_scanned_dirs()->Reserve(scanned_dirs.size());
    for (const auto& d : scanned_dirs) {
      *scan_status->mutable_scanned_dirs()->Add() = d;
    }
  }

  void AddFileItem(const std::string& path, const proto::FileType type,
                   proto::ScanStatus* scan_status) {
    proto::FileItem* file_item = scan_status->mutable_scanned_files()->Add();
    file_item->set_path(path);
    int64_t create_time = 0, update_time = 0, size = 0;
    Util::FileInfo(path, &create_time, &update_time, &size);
    file_item->set_create_time(create_time);
    file_item->set_update_time(update_time);
    file_item->set_size(size);
    file_item->set_file_type(type);
  }

  void AddFileItemWithLock(const std::string& path, const proto::FileType type,
                           proto::ScanStatus* scan_status) {
    proto::FileItem* file_item = nullptr;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      file_item = scan_status->mutable_scanned_files()->Add();
    }
    file_item->set_path(path);
    int64_t create_time = 0, update_time = 0, size = 0;
    Util::FileInfo(path, &create_time, &update_time, &size);
    file_item->set_create_time(create_time);
    file_item->set_update_time(update_time);
    file_item->set_size(size);
    file_item->set_file_type(type);
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

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (scanning_) {
        LOG(ERROR) << "Another scan is running ...";
        return false;
      }
      scanning_ = true;
    }

    Clear();

    if (!disable_scan_cache) {
      LoadCachedScanStatus(path, scan_status, scanned_dirs);
    }

    auto ret = Scan(path, scan_status, scanned_dirs);
    MarkDirStatus(scan_status, *scanned_dirs);
    if (ret) {
      ret = Dump(GenFileName(path), *scan_status);
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    scanning_ = false;
    return ret;
  }

  bool Scan(const std::string& path, proto::ScanStatus* scan_status,
            std::unordered_set<std::string>* scanned_dirs) {
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
      int symlink_num = 0;
      int file_num = 0;
      int dir_num = 0;

      for (const auto& entry : std::filesystem::directory_iterator(path)) {
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

      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink() || !entry.is_directory()) {
          continue;
        }

        if (!Scan(entry.path().string(), scan_status, scanned_dirs)) {
          return false;
        }
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

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (scanning_) {
        LOG(ERROR) << "Another scan is running ...";
        return false;
      }
      scanning_ = true;
    }

    Clear();

    if (!disable_scan_cache) {
      LoadCachedScanStatus(path, scan_status, scanned_dirs);
    }

    std::packaged_task<bool()> task(std::bind(
        static_cast<bool (ScanManager::*)(
            const std::string&, proto::ScanStatus*,
            std::unordered_set<std::string>*)>(&ScanManager::ParallelScan),
        this, path, scan_status, scanned_dirs));

    auto task_future = task.get_future();
    ++current_threads;
    ThreadPool::Instance()->Post(task);

    auto ret = task_future.get();
    MarkDirStatus(scan_status, *scanned_dirs);
    if (ret) {
      ret = Dump(GenFileName(path), *scan_status);
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    scanning_ = false;
    return ret;
  }

  bool ParallelScan(const std::string& path, proto::ScanStatus* scan_status,
                    std::unordered_set<std::string>* scanned_dirs) {
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

    int symlink_num = 0;
    int file_num = 0;
    int dir_num = 0;

    // LOG(INFO) << "Now scan " << path;
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          ++symlink_num;
          AddFileItemWithLock(entry.path().string(), proto::FileType::Symlink,
                              scan_status);
        } else if (entry.is_regular_file()) {
          ++file_num;
          AddFileItemWithLock(entry.path().string(), proto::FileType::Regular,
                              scan_status);
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

    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }

    bool complete = true;
    for (auto it = std::filesystem::directory_iterator(path);
         it != std::filesystem::directory_iterator();) {
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

    if (complete) {
      absl::base_internal::SpinLockHolder locker(&lock_);
      scanned_dirs->insert(path);
    }
    return complete;
  }

  void Print(const proto::ScanStatus& scan_status) {
    LOG(INFO) << "file_num: " << scan_status.file_num()
              << ", dir_num: " << scan_status.dir_num() + 1  // self
              << ", symlink_num: " << scan_status.symlink_num()
              << ", scanned_files num: " << scan_status.scanned_files().size()
              << ", scanned_dirs num: " << scan_status.scanned_dirs().size()
              << ", ignored_dirs num: " << scan_status.ignored_dirs().size();
  }

  void Clear() {
    scanning_ = false;
    current_threads = 0;
  }

 public:
  const static std::string mark_dir_name;

 private:
  mutable absl::base_internal::SpinLock lock_;
  bool scanning_ = false;
  int current_threads = 0;
  const int max_threads = 3;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
