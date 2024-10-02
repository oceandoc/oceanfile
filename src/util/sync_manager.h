/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H

#include <memory>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "src/proto/data.pb.h"
#include "src/util/scan_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class SyncManager {
 private:
  friend class folly::Singleton<SyncManager>;
  SyncManager() = default;

 public:
  static std::shared_ptr<SyncManager> Instance();

  bool Init() { return true; }

  bool SyncLocal(const std::string& src, const std::string& dst,
                 bool disable_scan_cache = false) {
    if (!Util::IsAbsolute(src) || !Util::IsAbsolute(dst)) {
      LOG(ERROR) << "path must be absolute";
      return false;
    }

    std::string canonical_src(src);
    std::string canonical_dst(dst);
    Util::UnifyDir(&canonical_src);
    Util::UnifyDir(&canonical_dst);

    if (Util::StartWith(canonical_dst, canonical_src)) {
      LOG(ERROR) << "Cannot sync " << canonical_src << " to " << canonical_dst
                 << ", for cannot sync to subdir";
      return false;
    }
    Util::Mkdir(canonical_dst);

    proto::ScanStatus scan_status;
    std::unordered_set<std::string> copy_failed_files;
    std::unordered_set<std::string> scanned_dirs;
    scan_status.mutable_ignored_dirs()->insert({common::CONFIG_DIR, true});

    auto ret = ScanManager::Instance()->ParallelScan(
        canonical_src, &scan_status, &scanned_dirs, disable_scan_cache);
    if (!ret) {
      LOG(ERROR) << "Scan " << src << " error";
      return false;
    }

    bool stop_dump_task = false;
    auto dump_task =
        std::bind(&ScanManager::DumpTask, ScanManager::Instance().get(),
                  &stop_dump_task, canonical_src, &scan_status, &scanned_dirs);
    ThreadPool::Instance()->Post(dump_task);

    bool success = true;
    std::vector<std::future<bool>> rets;
    for (int i = 0; i < max_threads; ++i) {
      std::packaged_task<bool()> task(
          std::bind(&SyncManager::SyncWorker, this, i, &scan_status,
                    canonical_src, canonical_dst, &copy_failed_files));
      rets.emplace_back(task.get_future());
      ThreadPool::Instance()->Post(task);
    }
    for (auto& f : rets) {
      if (f.get() == false) {
        success = false;
      }
    }

    stop_dump_task = true;
    ScanManager::Instance()->Print(scan_status);
    ScanManager::Instance()->Dump(canonical_src, &scan_status, scanned_dirs);
    SyncStatusDir(canonical_src, canonical_dst);

    for (const auto& file : copy_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }
    return success;
  }

 private:
  bool SyncWorker(const int no, proto::ScanStatus* status,
                  const std::string& src, const std::string& dst,
                  std::unordered_set<std::string>* copy_failed_files) {
    bool success = true;
    for (int i = no; i < status->scanned_files().size(); i += max_threads) {
      if (status->scanned_files(i).sync_finished()) {
        continue;
      }

      std::string relative_path;
      Util::Relative(status->scanned_files(i).path(), src, &relative_path);
      auto dst_path = std::filesystem::path(dst + "/" + relative_path);
      Util::MkParentDir(dst_path);
      bool ret = true;
      if (status->scanned_files(i).file_type() == proto::Symlink) {
        ret = Util::SyncSymlink(src, dst, status->scanned_files(i).path());
      } else {
        ret = Util::CopyFile(status->scanned_files(i).path(), dst_path.string(),
                             std::filesystem::copy_options::overwrite_existing);
      }
      if (!ret) {
        LOG(ERROR) << "Sync error: " << status->scanned_files(i).path();
        absl::base_internal::SpinLockHolder locker(&lock_);
        copy_failed_files->insert(status->scanned_files(i).path());
        success = false;
      }
      status->mutable_scanned_files(i)->set_sync_finished(true);
    }

    for (int i = no; i < status->scanned_dirs().size(); i += max_threads) {
      std::filesystem::path dir = status->scanned_dirs(i);

      if (!std::filesystem::is_directory(dir)) {
        LOG(ERROR) << dir.string() << " treated as dir wrong";
        continue;
      }

      if (dir.filename() == common::CONFIG_DIR) {
        continue;
      }

      std::string relative_path;
      Util::Relative(dir.string(), src, &relative_path);
      auto dst_path = std::filesystem::path(dst + "/" + relative_path);

      if (std::filesystem::is_symlink(dir)) {
        LOG(ERROR) << dir.string() << " symlink";
      }

      if (Util::Exists(dst_path.string())) {
        continue;
      }
      std::filesystem::create_directories(dst_path);
    }
    return success;
  }

  void SyncStatusDir(const std::string& canonical_src,
                     const std::string& canonical_dst) {
    const auto& src = ScanManager::Instance()->GenFileDir(canonical_src);
    const auto& dst = canonical_dst + "/" + common::CONFIG_DIR;
    Util::Copy(src, dst);
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> scanning_ = false;
  std::atomic<bool> stop_ = false;
  int current_threads = 0;
  const int max_threads = 5;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
