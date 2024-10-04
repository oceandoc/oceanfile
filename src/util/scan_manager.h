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
#include <set>
#include <string>
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

  bool LoadCachedScanStatus(const std::string& path,
                            proto::ScanStatus* scan_status) {
    const std::string& cached_status_path = GenFileName(path);
    if (!Util::Exists(cached_status_path)) {
      return false;
    }

    std::string content, decompressed_content;
    if (Util::LoadSmallFile(cached_status_path, &content)) {
      LOG(INFO) << "Load cached scan_status success: " << cached_status_path;
      if (!Util::LZMADecompress(content, &decompressed_content)) {
        return false;
      }
      if (!scan_status->ParseFromString(decompressed_content)) {
        return false;
      }
      Print(*scan_status);
      return true;
    }
    return false;
  }

  proto::ErrCode Dump(const std::string& path, proto::ScanStatus* scan_status) {
    if (scan_status->uuid().empty()) {
      scan_status->set_uuid(Util::UUID());
    }

    auto rename_ret = Util::Rename(path, path + ".tmp");
    std::string content, compressed_content;
    scan_status->SerializeToString(&content);
    Util::LZMACompress(content, &compressed_content);
    if (Util::WriteToFile(path, compressed_content, false) ||
        Util::WriteToFile(path, compressed_content, false)) {
      return proto::ErrCode::Success;
    }
    if (rename_ret) {
      Util::Rename(path + ".tmp", path);
    }
    return proto::ErrCode::Scan_dump_error;
  }

  void DumpTask(bool* stop, const std::string& path,
                proto::ScanStatus* scan_status) {
    while (!(*stop)) {
      std::unique_lock<std::mutex> lock(mu_);
      if (cond_var_.wait_for(lock, std::chrono::minutes(2),
                             [stop] { return *stop; })) {
        break;
      }
      Dump(GenFileName(path), scan_status);
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

  void Print(const proto::ScanStatus& scan_status) {
    LOG(INFO) << "scanned_dirs num: " << scan_status.scanned_dirs().size()
              << ", scanned_files num: " << scan_status.scanned_files().size()
              << ", ignored_dirs num: " << scan_status.ignored_dirs().size();
  }

  proto::ErrCode AddDirItem(const std::string& path,
                            proto::ScanStatus* scan_status) {
    proto::DirItem dir_item;
    dir_item.set_path(path);
    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      return proto::ErrCode::File_permission_or_not_exists;
    }

    dir_item.set_update_time(update_time);
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      scan_status->mutable_scanned_dirs()->insert({path, dir_item});
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode AddFileItem(const std::string& path,
                             const proto::FileType type,
                             proto::ScanStatus* scan_status,
                             const bool calc_hash) {
    proto::FileItem file_item;
    file_item.set_path(path);
    int64_t update_time = 0, size = 0;
    if (!Util::FileInfo(path, &update_time, &size)) {
      return proto::ErrCode::File_permission_or_not_exists;
    }

    if (calc_hash) {
      if (!Util::FileSHA256(path, file_item.mutable_sha256())) {
        return proto::ErrCode::File_sha256_calc_error;
      }
    }

    file_item.set_update_time(update_time);
    file_item.set_size(size);
    file_item.set_file_type(type);
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      scan_status->mutable_scanned_files()->insert({path, file_item});
      std::filesystem::path s_path(path);
      auto it = scan_status->mutable_scanned_dirs()->find(
          s_path.parent_path().string());
      it->second.mutable_files()->insert({path, update_time});
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode RemoveDir(proto::ScanStatus* scan_status,
                           const std::string& dir) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = scan_status->mutable_scanned_dirs()->find(dir);
    for (const auto& p : it->second.files()) {
      scan_status->mutable_scanned_files()->erase(p.first);
    }
    scan_status->mutable_scanned_dirs()->erase(dir);
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelScan(const std::string& path,
                              proto::ScanStatus* scan_status,
                              const bool calc_hash,
                              const bool disable_scan_cache) {
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

    bool loaded_cached_status = false;
    if (!disable_scan_cache) {
      loaded_cached_status = LoadCachedScanStatus(path, scan_status);
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(&ScanManager::DumpTask, this, &stop_dump_task,
                               path, scan_status);
    ThreadPool::Instance()->Post(dump_task);

    proto::ErrCode ret = proto::ErrCode::Success;
    std::vector<std::string> dirs;
    if (loaded_cached_status) {
      std::vector<std::future<proto::ErrCode>> rets;
      dirs.reserve(scan_status->scanned_dirs().size());
      for (const auto& p : scan_status->scanned_dirs()) {
        dirs.push_back(p.first);
      }
      for (int32_t i = 0; i < max_threads; ++i) {
        std::packaged_task<proto::ErrCode()> task(std::bind(
            static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
                const int32_t thread_no, proto::ScanStatus*,
                const std::vector<std::string>*, const bool calc_hash)>(
                &ScanManager::ParallelScanWithCache),
            this, i, scan_status, &dirs, calc_hash));
        ThreadPool::Instance()->Post(task);
      }

      for (auto& f : rets) {
        if (f.get() != proto::ErrCode::Success) {
          ret = f.get();
        }
      }
    } else {
      std::packaged_task<proto::ErrCode()> task(std::bind(
          static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
              const std::string&, proto::ScanStatus*, const bool calc_hash)>(
              &ScanManager::ParallelFullScan),
          this, path, scan_status, calc_hash));

      auto task_future = task.get_future();
      ++current_threads;
      ThreadPool::Instance()->Post(task);
      ret = task_future.get();
    }

    stop_dump_task = true;
    cond_var_.notify_all();
    current_threads = 0;
    if (ret != proto::ErrCode::Success) {
      scanning_.store(false);
      return ret;
    }

    ret = Dump(GenFileName(path), scan_status);
    scanning_.store(false);
    if (ret == proto::ErrCode::Success) {
      scan_status->set_complete_time(Util::CurrentTimeMillis());
    }
    return ret;
  }

  proto::ErrCode ParallelScanWithCache(const int32_t thread_no,
                                       proto::ScanStatus* scan_status,
                                       const std::vector<std::string>* dirs,
                                       const bool calc_hash) {
    for (size_t i = thread_no; i < dirs->size(); i += max_threads) {
      if (stop_.load()) {
        return proto::ErrCode::Scan_interrupted;
      }

      auto ret = proto::ErrCode::Success;
      const auto& dir = (*dirs)[i];
      if (!Util::Exists(dir)) {
        RemoveDir(scan_status, dir);
        continue;
      }

      auto update_time = Util::UpdateTime((*dirs)[i]);
      auto it = scan_status->mutable_scanned_dirs()->find(dir);
      if (it->second.update_time() == update_time) {
        continue;
      }
      it->second.set_update_time(update_time);

      ret = Scan(dir, scan_status, calc_hash);
      if (ret != proto::ErrCode::Success) {
        return ret;
      }
    }

    return proto::ErrCode::Success;
  }

  proto::ErrCode Scan(const std::string& path, proto::ScanStatus* scan_status,
                      const bool calc_hash) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    std::set<std::string> files;
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        files.insert(entry.path().string());
        auto ret = proto::ErrCode::Success;
        if (entry.is_symlink()) {
          ret = AddFileItem(entry.path().string(), proto::FileType::Symlink,
                            scan_status, calc_hash);
        } else if (entry.is_regular_file()) {
          ret = AddFileItem(entry.path().string(), proto::FileType::Regular,
                            scan_status, calc_hash);
        } else if (entry.is_directory()) {
          {
            absl::base_internal::SpinLockHolder locker(&lock_);
            auto it = scan_status->scanned_dirs().find(entry.path().string());
            if (it != scan_status->scanned_dirs().end()) {
              continue;
            }
          }
          ret = AddDirItem(entry.path().string(), scan_status);
          ret = Scan(entry.path().string(), scan_status, calc_hash);
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

    auto it = scan_status->scanned_dirs().find(path);
    if (it == scan_status->scanned_dirs().end()) {
      LOG(ERROR) << "This should never happen";
    }

    auto dir_item = it->second;
    for (auto file_it = dir_item.mutable_files()->begin();
         file_it != dir_item.mutable_files()->end();) {
      if (files.find(file_it->first) == files.end()) {
        file_it = dir_item.mutable_files()->erase(file_it);
      } else {
        ++file_it;
      }
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelFullScan(const std::string& path,
                                  proto::ScanStatus* scan_status,
                                  const bool calc_hash) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    auto it = scan_status->ignored_dirs().find(path);
    if (it != scan_status->ignored_dirs().end()) {
      return proto::ErrCode::Success;
    }

    LOG(INFO) << "Now scan " << path;
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
      LOG(INFO) << "current_threads: " << current_threads;
      std::vector<std::future<proto::ErrCode>> rets;
      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        while (current_threads <= max_threads &&
               it != std::filesystem::directory_iterator()) {
          if (it->is_symlink() || !it->is_directory()) {
            ++it;
            continue;
          }
          std::packaged_task<proto::ErrCode()> task(std::bind(
              static_cast<proto::ErrCode (ScanManager::*)(  // NOLINT
                  const std::string&, proto::ScanStatus*, const bool)>(
                  &ScanManager::ParallelFullScan),
              this, it->path().string(), scan_status, calc_hash));
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

      auto ret = ParallelFullScan(cur_thread_next_dir, scan_status, calc_hash);
      if (ret != proto::ErrCode::Success) {
        all_ret = ret;
      }
    }

    if (all_ret != proto::ErrCode::Success) {
      return all_ret;
    }

    AddDirItem(path, scan_status);

    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        auto ret = proto::ErrCode::Success;
        if (entry.is_symlink()) {
          ret = AddFileItem(entry.path().string(), proto::FileType::Symlink,
                            scan_status, calc_hash);
        } else if (entry.is_regular_file()) {
          ret = AddFileItem(entry.path().string(), proto::FileType::Regular,
                            scan_status, calc_hash);
        } else if (entry.is_directory()) {
          ret = AddDirItem(entry.path().string(), scan_status);
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
  const int max_threads = 4;
  std::mutex mu_;
  std::condition_variable cond_var_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SCAN_MANAGER_H
