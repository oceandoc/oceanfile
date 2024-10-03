/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H

#include <filesystem>
#include <memory>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "src/proto/data.pb.h"
#include "src/proto/error.pb.h"
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

  proto::ErrCode SyncLocal(const std::string& src, const std::string& dst,
                           const bool calc_hash = false,
                           const bool disable_scan_cache = false) {
    if (!Util::IsAbsolute(src) || !Util::IsAbsolute(dst)) {
      LOG(ERROR) << "Path must be absolute";
      return proto::ErrCode::Path_not_absolute;
    }

    std::string unify_src(src);
    std::string unify_dst(dst);
    Util::UnifyDir(&unify_src);
    Util::UnifyDir(&unify_dst);

    if (Util::StartWith(unify_dst, unify_src)) {
      LOG(ERROR) << "Cannot sync " << unify_src << " to " << unify_dst
                 << ", for cannot sync to subdir";
      return proto::ErrCode::Path_dst_is_src_subdir;
    }

    if (!Util::Exists(unify_src) || !Util::Exists(unify_dst)) {
      LOG(ERROR) << "Src or dst not exists";
      return proto::ErrCode::Path_not_exists;
    }

    if (!std::filesystem::is_directory(unify_src) ||
        !std::filesystem::is_directory(unify_dst)) {
      LOG(ERROR) << "Src or dst not dir";
      return proto::ErrCode::Path_not_dir;
    }

    proto::ScanStatus scan_status;
    std::unordered_set<std::string> copy_failed_files;
    DirUpTimeMap dirs_uptime;
    DirMap scanned_dirs;
    // scan_status.mutable_ignored_dirs()->insert(
    // {unify_src + "/" + common::CONFIG_DIR, true});

    auto ret = ScanManager::Instance()->ParallelScan(
        unify_src, &scan_status, &dirs_uptime, &scanned_dirs, calc_hash,
        disable_scan_cache);
    if (!ret) {
      LOG(ERROR) << "Scan " << src << " error";
      return proto::ErrCode::Scan_error;
    }

    bool stop_dump_task = false;
    auto dump_task = std::bind(
        &ScanManager::DumpTask, ScanManager::Instance().get(), &stop_dump_task,
        unify_src, &scan_status, &dirs_uptime, &scanned_dirs);
    ThreadPool::Instance()->Post(dump_task);

    bool success = true;
    std::vector<std::future<bool>> rets;
    for (int i = 0; i < max_threads; ++i) {
      std::packaged_task<bool()> task(std::bind(&SyncManager::SyncWorker, this,
                                                i, &scan_status, unify_src,
                                                unify_dst, &copy_failed_files));
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
    ScanManager::Instance()->Dump(unify_src, &scan_status, dirs_uptime,
                                  scanned_dirs);
    SyncStatusDir(unify_src, unify_dst);

    for (const auto& file : copy_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }
    if (success) {
      return proto::ErrCode::Success;
    }
    return proto::ErrCode::Fail;
  }

 private:
  bool SyncWorker(const int no, proto::ScanStatus* status,
                  const std::string& src, const std::string& dst,
                  std::unordered_set<std::string>* copy_failed_files) {
    bool success = true;
    for (int i = no; i < status->scanned_files().size(); i += max_threads) {
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
    }

    for (int i = no; i < status->scanned_dirs().size(); i += max_threads) {
      std::filesystem::path dir = status->scanned_dirs(i).path();

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

  void SyncStatusDir(const std::string& unify_src,
                     const std::string& unify_dst) {
    const auto& src = ScanManager::Instance()->GenFileDir(unify_src);
    const auto& dst = unify_dst + "/" + common::CONFIG_DIR;
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
