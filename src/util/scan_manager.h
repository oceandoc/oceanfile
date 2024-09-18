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
    return path + "/.Dr.Q.config";
  }

  std::string GenFileName(const std::string& path) {
    return GenFileDir(path) + "/" + Util::ToHexStr(Util::Hash64(path));
  }

  // TODO remark complete status here
  void LoadCachedScanStatus(const std::string& path) {
    scanned_dirs_.emplace(std::move(GenFileDir(path)));
    const std::string cached_status_path = GenFileName(path);
    if (!std::filesystem::exists(cached_status_path)) {
      return;
    }
    std::string content;
    if (Util::LoadSmallFile(cached_status_path, &content)) {
      LOG(INFO) << "Load cached status: " << cached_status_path;
      status_.ParseFromString(content);
      Print();
      for (const auto& scanned_dir : status_.scanned_dirs()) {
        scanned_dirs_.emplace(scanned_dir);
      }
    }
  }

  bool Dump(const std::string& path) {
    Util::TruncateFile(path);
    std::string content;
    status_.SerializeToString(&content);
    return Util::WriteToFile(path, content, true);
  }

  proto::ScanStatus Scan(const std::string& path,
                         bool disable_scan_cache = false) {
    if (!std::filesystem::exists(path)) {
      LOG(ERROR) << path << " not exists";
      return proto::ScanStatus();
    }

    if (!std::filesystem::is_directory(path)) {
      LOG(ERROR) << path << " not directory";
      return proto::ScanStatus();
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (scanning_) {
        LOG(ERROR) << "Another scan is running ...";
        return proto::ScanStatus();
      }
      scanning_ = true;
    }

    Clear();
    if (!disable_scan_cache) {
      LoadCachedScanStatus(path);
    }

    auto ret = Scan(path, &status_);
    MarkDirStatus();
    if (ret) {
      ret = Dump(GenFileName(path));
      status_.set_complete_time(Util::Now());
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    scanning_ = false;
    return status_;
  }

  bool Scan(const std::string& path, proto::ScanStatus* scan_status) {
    auto it = scanned_dirs_.find(path);
    if (it != scanned_dirs_.end()) {
      return true;
    }

    try {
      int symlink_num = 0;
      int file_num = 0;
      int dir_num = 0;

      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        auto file_item = scan_status->mutable_scanned_files()->Add();
        file_item->set_path(entry.path().string());
        int64_t create_time = 0, update_time = 0, size = 0;
        Util::FileInfo(entry.path().string(), &create_time, &update_time,
                       &size);
        file_item->set_create_time(create_time);
        file_item->set_update_time(update_time);
        file_item->set_size(size);

        if (entry.is_symlink()) {
          ++symlink_num;
          file_item->set_file_type(proto::FileType::Symlink);
        } else if (entry.is_regular_file()) {
          ++file_num;
          file_item->set_file_type(proto::FileType::Regular);
        } else if (entry.is_directory()) {
          file_item->set_file_type(proto::FileType::Dir);
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

        if (!Scan(entry.path().string(), scan_status)) {
          return false;
        }
      }

      scanned_dirs_.insert(path);
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }
    return true;
  }

  void MarkDirStatus() {
    status_.mutable_scanned_dirs()->Reserve(scanned_dirs_.size());
    for (const auto& d : scanned_dirs_) {
      *status_.mutable_scanned_dirs()->Add() = d;
    }
  }

  proto::ScanStatus ParallelScan(const std::string& path,
                                 bool disable_scan_cache = false) {
    if (!std::filesystem::exists(path)) {
      LOG(ERROR) << path << " not exists";
      return proto::ScanStatus();
    }

    if (!std::filesystem::is_directory(path)) {
      LOG(ERROR) << path << " not directory";
      return proto::ScanStatus();
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (scanning_) {
        LOG(ERROR) << "Another scan is running ...";
        return proto::ScanStatus();
      }
      scanning_ = true;
    }

    Clear();
    if (!disable_scan_cache) {
      LoadCachedScanStatus(path);
    }

    std::packaged_task<bool()> task(
        std::bind(static_cast<bool (ScanManager::*)(const std::string&,
                                                    proto::ScanStatus*)>(
                      &ScanManager::ParallelScan),
                  this, path, &status_));
    auto task_future = task.get_future();
    ++current_threads;
    ThreadPool::Instance()->Post(task);

    auto ret = task_future.get();
    MarkDirStatus();
    if (ret) {
      ret = Dump(GenFileName(path));
      status_.set_complete_time(Util::Now());
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    scanning_ = false;
    return status_;
  }

  bool ParallelScan(const std::string& path, proto::ScanStatus* scan_status) {
    auto it = scanned_dirs_.find(path);
    if (it != scanned_dirs_.end()) {
      return true;
    }

    int symlink_num = 0;
    int file_num = 0;
    int dir_num = 0;

    // LOG(INFO) << "Now scan " << path;
    try {
      proto::FileItem* file_item = nullptr;
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        {
          absl::base_internal::SpinLockHolder locker(&lock_);
          file_item = scan_status->mutable_scanned_files()->Add();
        }
        file_item->set_path(entry.path().string());
        int64_t create_time = 0, update_time = 0, size = 0;
        Util::FileInfo(entry.path().string(), &create_time, &update_time,
                       &size);
        file_item->set_create_time(create_time);
        file_item->set_update_time(update_time);
        file_item->set_size(size);

        if (entry.is_symlink()) {
          ++symlink_num;
          file_item->set_file_type(proto::FileType::Symlink);
        } else if (entry.is_regular_file()) {
          ++file_num;
          file_item->set_file_type(proto::FileType::Regular);
        } else if (entry.is_directory()) {
          file_item->set_file_type(proto::FileType::Dir);
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
              std::bind(static_cast<bool (ScanManager::*)(const std::string&,
                                                          proto::ScanStatus*)>(
                            &ScanManager::ParallelScan),
                        this, it->path().string(), scan_status));
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

      if (!ParallelScan(cur_thread_next_dir, scan_status)) {
        complete = false;
      }
    }

    if (complete) {
      scanned_dirs_.insert(path);
    }
    return complete;
  }

  void Print() {
    LOG(INFO) << "file_num: " << status_.file_num()
              << ", dir_num: " << status_.dir_num()
              << ", symlink_num: " << status_.symlink_num();
  }

  void Clear() {
    status_.Clear();
    scanned_dirs_.clear();
    scanning_ = false;
    current_threads = 0;
    status_.mutable_scanned_files()->Reserve(10000000);
    scanned_dirs_.reserve(10000000);
  }

  proto::ScanStatus GetStatus() { return status_; }

 private:
  mutable absl::base_internal::SpinLock lock_;
  proto::ScanStatus status_;
  std::unordered_set<std::string> scanned_dirs_;
  bool scanning_ = false;
  int current_threads = 0;
  const int max_threads = 3;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
