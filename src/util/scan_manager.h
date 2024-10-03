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
#include <unordered_map>
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

using DirUpTimeMap = std::unordered_map<std::string, int64_t>;
using DirMap =
    std::unordered_map<std::string,
                       std::unordered_map<std::string, proto::FileItem>>;
using FileMap = std::unordered_map<std::string, proto::FileItem>;

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

  void InsertFileItem(DirMap* scanned_dirs, const proto::FileItem& file_item) {
    std::filesystem::path s_path(file_item.path());
    auto it = scanned_dirs->find(s_path.parent_path().string());
    if (it != scanned_dirs->end()) {
      it->second.insert({file_item.path(), file_item});
    }
    FileMap file_map{{file_item.path(), file_item}};
    scanned_dirs->insert({s_path.parent_path().string(), file_map});
  }

  bool LoadCachedScanStatus(const std::string& path,
                            proto::ScanStatus* scan_status,
                            DirUpTimeMap* dirs_uptime, DirMap* scanned_dirs) {
    const std::string& cached_status_path = GenFileName(path);
    if (!Util::Exists(cached_status_path)) {
      return false;
    }

    std::string content, decompressed_content;
    if (Util::LoadSmallFile(cached_status_path, &content)) {
      LOG(INFO) << "Load cached scan_status success: " << cached_status_path;
      Util::LZMADecompress(content, &decompressed_content);
      scan_status->ParseFromString(decompressed_content);
      Print(*scan_status);

      for (const auto& dir_item : scan_status->scanned_dirs()) {
        dirs_uptime->insert({dir_item.path(), dir_item.update_time()});
      }

      for (const auto& file_item : scan_status->scanned_files()) {
        InsertFileItem(scanned_dirs, file_item);
      }
      return true;
    }
    return false;
  }

  void UpdateScanStatus(proto::ScanStatus* scan_status,
                        const DirUpTimeMap& dirs_uptime,
                        const DirMap& scanned_dirs) {
    scan_status->mutable_scanned_dirs()->Clear();
    scan_status->mutable_scanned_dirs()->Reserve(dirs_uptime.size());
    scan_status->mutable_scanned_files()->Clear();
    scan_status->mutable_scanned_files()->Reserve(scanned_dirs.size());
    absl::base_internal::SpinLockHolder locker(&lock_);
    for (const auto& d : dirs_uptime) {
      auto dir_item = *scan_status->mutable_scanned_dirs()->Add();
      dir_item.set_path(d.first);
      dir_item.set_update_time(d.second);
    }
    for (const auto& dir : scanned_dirs) {
      for (const auto& file : dir.second) {
        *scan_status->mutable_scanned_files()->Add() = file.second;
      }
    }
  }

  proto::ErrCode Dump(const std::string& path, proto::ScanStatus* scan_status,
                      const DirUpTimeMap& dirs_uptime,
                      const DirMap& scanned_dirs) {
    UpdateScanStatus(scan_status, dirs_uptime, scanned_dirs);
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

  void DumpTask(const bool* const stop, const std::string& path,
                proto::ScanStatus* scan_status, DirUpTimeMap* dirs_uptime,
                DirMap* scanned_dirs) {
    absl::Mutex mtx;
    while (!*stop) {
      absl::MutexLock lock(&mtx);
      mtx.AwaitWithTimeout(absl::Condition(stop), absl::Seconds(120));
      Dump(GenFileName(path), scan_status, *dirs_uptime, *scanned_dirs);
    }
    LOG(INFO) << "DumpTask Exists";
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
    LOG(INFO) << "scanned_dirs num: " << scan_status.scanned_dirs().size()
              << ", scanned_files num: " << scan_status.scanned_files().size()
              << ", ignored_dirs num: " << scan_status.ignored_dirs().size();
  }

  proto::ErrCode AddFileItemWithLock(const std::string& path,
                                     const proto::FileType type,
                                     DirMap* scanned_dirs,
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
      InsertFileItem(scanned_dirs, file_item);
    }
    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelScan(const std::string& path,
                              proto::ScanStatus* scan_status,
                              DirUpTimeMap* dirs_uptime, DirMap* scanned_dirs,
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
      loaded_cached_status =
          LoadCachedScanStatus(path, scan_status, dirs_uptime, scanned_dirs);
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(&ScanManager::DumpTask, this, &stop_dump_task,
                               path, scan_status, dirs_uptime, scanned_dirs);
    ThreadPool::Instance()->Post(dump_task);

    proto::ErrCode ret = proto::ErrCode::Success;
    if (loaded_cached_status) {
      std::vector<std::future<proto::ErrCode>> rets;
      for (int i = 0; i < max_threads; ++i) {
        std::packaged_task<proto::ErrCode()> task(std::bind(
            static_cast<proto::ErrCode (ScanManager::*)(
                const int32_t thread_no, const proto::ScanStatus&,
                DirUpTimeMap*, DirMap*, const bool calc_hash)>(
                &ScanManager::ParallelScanWithCache),
            this, i, *scan_status, dirs_uptime, scanned_dirs, calc_hash));
        ThreadPool::Instance()->Post(task);
      }

      for (auto& f : rets) {
        if (f.get() != proto::ErrCode::Success) {
          ret = f.get();
        }
      }
    } else {
      std::packaged_task<proto::ErrCode()> task(std::bind(
          static_cast<proto::ErrCode (ScanManager::*)(
              const std::string&, proto::ScanStatus*, DirUpTimeMap*, DirMap*,
              const bool calc_hash)>(&ScanManager::ParallelFullScan),
          this, path, scan_status, dirs_uptime, scanned_dirs, calc_hash));

      auto task_future = task.get_future();
      ++current_threads;
      ThreadPool::Instance()->Post(task);
      ret = task_future.get();
    }

    if (ret == proto::ErrCode::Success) {
      ret = Dump(GenFileName(path), scan_status, *dirs_uptime, *scanned_dirs);
      if (ret == proto::ErrCode::Success) {
        scan_status->set_complete_time(Util::CurrentTimeMillis());
      }
    }
    current_threads = 0;
    scanning_.store(false);
    return ret;
  }

  proto::ErrCode ReScanDir(const std::string& path, DirUpTimeMap* dirs_uptime,
                           DirMap* scanned_dirs, const bool calc_hash) {
    proto::ErrCode ret = proto::ErrCode::Success;
    try {
      std::set<std::string> files;
      std::set<std::string> dirs;
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          files.insert(entry.path().string());
          auto ret = AddFileItemWithLock(entry.path().string(),
                                         proto::FileType::Symlink, scanned_dirs,
                                         calc_hash);
          if (ret != proto::ErrCode::Success) {
            return ret;
          }
        } else if (entry.is_regular_file()) {
          files.insert(entry.path().string());
          auto ret = AddFileItemWithLock(entry.path().string(),
                                         proto::FileType::Regular, scanned_dirs,
                                         calc_hash);
          if (ret != proto::ErrCode::Success) {
            return ret;
          }
        } else if (entry.is_directory()) {
          files.insert(entry.path().string());
          {
            absl::base_internal::SpinLockHolder locker(&lock_);
            if (dirs_uptime->find(entry.path().string()) !=
                dirs_uptime->end()) {
              continue;
            }
          }
          ret = ReScanDir(entry.path().string(), dirs_uptime, scanned_dirs,
                          calc_hash);
          if (ret == proto::ErrCode::Success) {
            auto update_time = Util::UpdateTime(path);
            absl::base_internal::SpinLockHolder locker(&lock_);
            dirs_uptime->insert({entry.path().string(), update_time});
          }
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return proto::ErrCode::Fail;
    }

    if (ret != proto::ErrCode::Success) {
      return ret;
    }

    auto it = scanned_dirs->find(path);
    if (it == scanned_dirs->end()) {
      LOG(ERROR) << "This should never happen";
    }
    auto& dir_map = it->second;
    for (auto it = dir_map.begin(); it != dir_map.end()) {
    }
  }

  proto::ErrCode ParallelScanWithCache(const int32_t thread_no,
                                       const proto::ScanStatus& scan_status,
                                       DirUpTimeMap* dirs_uptime,
                                       DirMap* scanned_dirs,
                                       const bool calc_hash) {
    for (int i = thread_no; i < scan_status.scanned_dirs().size();
         i += max_threads) {
      const auto& dir_item = scan_status.scanned_dirs(i);
      auto update_time = Util::UpdateTime(dir_item.path());
      if (dir_item.update_time() == update_time) {
        continue;
      }
      auto ret =
          ReScanDir(dir_item.path(), dirs_uptime, scanned_dirs, calc_hash);
      if (ret != proto::ErrCode::Success) {
        return ret;
      }
    }

    return proto::ErrCode::Success;
  }

  proto::ErrCode ParallelFullScan(const std::string& path,
                                  proto::ScanStatus* scan_status,
                                  DirUpTimeMap* dirs_uptime,
                                  DirMap* scanned_dirs, const bool calc_hash) {
    if (stop_.load()) {
      return proto::ErrCode::Scan_interrupted;
    }

    auto it = scan_status->ignored_dirs().find(path);
    if (it != scan_status->ignored_dirs().end()) {
      return proto::ErrCode::Success;
    }

    // LOG(INFO) << "Now scan " << path;
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
          std::packaged_task<proto::ErrCode()> task(std::bind(
              static_cast<proto::ErrCode (ScanManager::*)(
                  const std::string&, proto::ScanStatus*, DirUpTimeMap*,
                  DirMap*, const bool)>(&ScanManager::ParallelFullScan),
              this, it->path().string(), scan_status, dirs_uptime, scanned_dirs,
              calc_hash));
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

      auto ret = ParallelFullScan(cur_thread_next_dir, scan_status, dirs_uptime,
                                  scanned_dirs, calc_hash);

      if (ret != proto::ErrCode::Success) {
        all_ret = ret;
      }
    }

    if (all_ret != proto::ErrCode::Success) {
      return proto::ErrCode::Fail;
    }

    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          auto ret = AddFileItemWithLock(entry.path().string(),
                                         proto::FileType::Symlink, scanned_dirs,
                                         calc_hash);
          if (ret != proto::ErrCode::Success) {
            return ret;
          }
        } else if (entry.is_regular_file()) {
          auto ret = AddFileItemWithLock(entry.path().string(),
                                         proto::FileType::Regular, scanned_dirs,
                                         calc_hash);
          if (ret != proto::ErrCode::Success) {
            return ret;
          }
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return proto::ErrCode::Fail;
    }

    auto update_time = Util::UpdateTime(path);
    absl::base_internal::SpinLockHolder locker(&lock_);
    dirs_uptime->insert({path, update_time});
    return proto::ErrCode::Success;
  }

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
