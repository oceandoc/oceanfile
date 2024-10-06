/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "src/common/defs.h"
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

  void Stop() { ScanManager::Instance()->Stop(); }

  proto::ErrCode SyncLocal(const std::string& src, const std::string& dst,
                           const common::HashMethod hash_method,
                           const common::SyncMethod sync_method,
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
    scan_status.set_path(unify_src);
    ScanContext ctx;
    ctx.src = unify_src;
    ctx.dst = unify_dst;
    ctx.status = &scan_status;
    ctx.hash_method = hash_method;
    ctx.sync_method = sync_method;
    ctx.disable_scan_cache = disable_scan_cache;

    auto ret = ScanManager::Instance()->ParallelScan(&ctx);
    if (ret != proto::ErrCode::Success) {
      LOG(ERROR) << "Scan " << src << " error";
      return ret;
    }

    bool success = true;
    std::unordered_set<std::string> copy_failed_files;
    std::vector<std::future<bool>> rets;
    for (int i = 0; i < max_threads; ++i) {
      std::packaged_task<bool()> task(
          std::bind(&SyncManager::SyncWorker, this, i, &ctx));
      rets.emplace_back(task.get_future());
      ThreadPool::Instance()->Post(task);
    }
    for (auto& f : rets) {
      if (f.get() == false) {
        success = false;
      }
    }

    SyncStatusDir(&ctx);

    for (const auto& file : copy_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }
    if (success) {
      return proto::ErrCode::Success;
    }
    return proto::ErrCode::Fail;
  }

 private:
  bool SyncWorker(const int thread_no, ScanContext* ctx) {
    bool success = true;
    for (const auto& p : ctx->status->scanned_files()) {
      auto hash = std::abs(Util::MurmurHash64A(p.first));
      if ((hash % max_threads) != thread_no) {
        continue;
      }

      if (Util::IsAbsolute(p.first)) {
        LOG(ERROR) << "Must be relative: " << p.first;
        continue;
      }

      const auto& src_path = ctx->src + "/" + p.first;
      const auto& dst_path = ctx->dst + "/" + p.first;
      if (!Util::Exists(src_path)) {
        LOG(ERROR) << src_path << " not exists";
        continue;
      }

      auto update_time = Util::UpdateTime(dst_path);
      if (update_time == p.second.update_time()) {
        continue;
      }

      Util::MkParentDir(dst_path);
      bool ret = true;
      if (p.second.file_type() == proto::Symlink) {
        ret = Util::SyncSymlink(ctx->src, ctx->dst, src_path);
        continue;
      } else {
        ret = Util::CopyFile(src_path, dst_path);
      }

      if (!ret) {
        LOG(ERROR) << "Sync error: " << p.first;
        absl::base_internal::SpinLockHolder locker(&lock_);
        ctx->copy_failed_files.push_back(p.first);
        success = false;
        continue;
      }

      ret = Util::SetUpdateTime(dst_path, p.second.update_time());
      if (!ret) {
        LOG(ERROR) << "Set update_time error: " << dst_path;
        absl::base_internal::SpinLockHolder locker(&lock_);
        ctx->copy_failed_files.push_back(p.first);
        success = false;
      }
    }

    for (const auto& p : ctx->status->scanned_dirs()) {
      auto hash = std::abs(Util::MurmurHash64A(p.first));
      if ((hash % max_threads) != thread_no) {
        continue;
      }

      const auto& src_path = ctx->src + "/" + p.first;
      const auto& dst_path = ctx->dst + "/" + p.first;

      if (!Util::Exists(src_path)) {
        LOG(ERROR) << src_path << " not exists";
        success = false;
        continue;
      }

      if (!std::filesystem::is_directory(src_path)) {
        LOG(ERROR) << src_path << " not dir";
        success = false;
        continue;
      }

      if (std::filesystem::is_symlink(src_path)) {
        success = false;
        LOG(ERROR) << src_path << " is symlink";
      }

      if (Util::Exists(dst_path)) {
        continue;
      }
      Util::Mkdir(dst_path);
    }

    if (ctx->sync_method == common::SyncMethod::Sync_SYNC) {
      for (const auto& p : ctx->removed_files) {
        auto hash = std::abs(Util::MurmurHash64A(p));
        if ((hash % max_threads) != thread_no) {
          continue;
        }

        if (Util::IsAbsolute(p)) {
          LOG(ERROR) << "Must be relative: " << p;
          continue;
        }

        const auto& src_path = ctx->src + "/" + p;
        const auto& dst_path = ctx->dst + "/" + p;
        if (Util::Exists(src_path)) {
          LOG(ERROR) << src_path << " exists";
          continue;
        }

        if (!Util::Remove(dst_path)) {
          LOG(ERROR) << "Delete " << src_path << " error";
          continue;
        }
      }
    }
    return success;
  }

  void SetDstScanStatus(ScanContext* ctx, proto::ScanStatus* dst_status) {
    const auto& src_status = *(ctx->status);
    dst_status->set_uuid(src_status.uuid());
    dst_status->set_path(ctx->dst);
    dst_status->set_update_time(src_status.update_time());
    dst_status->set_complete_time(src_status.complete_time());
    dst_status->mutable_ignored_dirs()->insert(
        src_status.ignored_dirs().begin(), src_status.ignored_dirs().end());

    dst_status->mutable_scanned_dirs()->insert(
        src_status.scanned_dirs().begin(), src_status.scanned_dirs().end());

    dst_status->mutable_scanned_files()->insert(
        src_status.scanned_files().begin(), src_status.scanned_files().end());
  }

  void SyncStatusDir(ScanContext* ctx) {
    const auto& dst_path = ScanManager::Instance()->GenFileName(ctx->dst);
    proto::ScanStatus dst_status;
    SetDstScanStatus(ctx, &dst_status);
    ctx->status = &dst_status;
    ScanManager::Instance()->Dump(dst_path, ctx);
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> scanning_ = false;
  std::atomic<bool> stop_ = false;
  int current_threads = 0;
  const int max_threads = 4;

  std::mutex mu_;
  std::condition_variable cond_var_;
  bool stop_fd_task_ = false;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
