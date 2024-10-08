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

  int32_t SyncLocal(const std::string& src, const std::string& dst,
                    const common::HashMethod hash_method,
                    const common::SyncMethod sync_method,
                    const bool disable_scan_cache = false,
                    const bool skip_scan = false) {
    if (!Util::IsAbsolute(src) || !Util::IsAbsolute(dst)) {
      LOG(ERROR) << "Path must be absolute";
      return Err_Path_not_absolute;
    }

    std::string unify_src(src);
    std::string unify_dst(dst);
    Util::UnifyDir(&unify_src);
    Util::UnifyDir(&unify_dst);

    if (Util::StartWith(unify_dst, unify_src)) {
      LOG(ERROR) << "Cannot sync " << unify_src << " to " << unify_dst
                 << ", for cannot sync to subdir";
      return Err_Path_dst_is_src_subdir;
    }

    if (!Util::Exists(unify_src) || !Util::Exists(unify_dst)) {
      LOG(ERROR) << "Src or dst not exists";
      return Err_Path_not_exists;
    }

    if (!std::filesystem::is_directory(unify_src) ||
        !std::filesystem::is_directory(unify_dst)) {
      LOG(ERROR) << "Src or dst not dir";
      return Err_Path_not_dir;
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
    ctx.skip_scan = skip_scan;

    auto ret = ScanManager::Instance()->ParallelScan(&ctx);
    if (ret != Err_Success) {
      LOG(ERROR) << "Scan " << src << " error";
      return ret;
    }

    bool success = true;
    syncd_total_cnt = 0;
    syncd_success_cnt = 0;
    syncd_fail_cnt = 0;
    syncd_skipped_cnt = 0;
    stop_progress_task_ = false;

    auto progress_task =
        std::bind(&SyncManager::ProgressTask, this, &stop_progress_task_, &ctx);
    ThreadPool::Instance()->Post(progress_task);

    LOG(INFO) << "Now sync " << ctx.src << " to " << ctx.dst;
    LOG(INFO) << "Memory usage: " << Util::MemUsage() << "MB";
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

    stop_progress_task_ = true;
    cond_var_.notify_all();

    SyncStatusDir(&ctx);

    for (const auto& file : copy_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }
    if (success) {
      LOG(INFO) << "Sync success";
      return Err_Success;
    }
    LOG(INFO) << "Sync failed";
    return Err_Fail;
  }

 private:
  bool SyncWorker(const int thread_no, ScanContext* ctx) {
    bool success = true;
    for (const auto& d : ctx->status->scanned_dirs()) {
      auto hash = std::abs(Util::MurmurHash64A(d.first));
      if ((hash % max_threads) != thread_no) {
        continue;
      }

      const auto& dir_src_path = ctx->src + "/" + d.first;
      const auto& dir_dst_path = ctx->dst + "/" + d.first;

      if (!Util::Exists(dir_src_path)) {
        LOG(ERROR) << dir_src_path << " not exists";
        success = false;
        continue;
      }

      if (!std::filesystem::is_directory(dir_src_path)) {
        LOG(ERROR) << dir_src_path << " not dir";
        success = false;
        continue;
      }

      if (std::filesystem::is_symlink(dir_src_path)) {
        success = false;
        LOG(ERROR) << dir_src_path << " is symlink";
      }

      if (!Util::Exists(dir_dst_path)) {
        Util::Mkdir(dir_dst_path);
      }

      if (!Util::SetUpdateTime(dir_dst_path, d.second.update_time())) {
        LOG(ERROR) << "Set update_time error: " << dir_dst_path;
        absl::base_internal::SpinLockHolder locker(&lock_);
        ctx->copy_failed_files.push_back(dir_src_path);
        syncd_fail_cnt.fetch_add(1);
        success = false;
      }

      for (const auto& f : d.second.files()) {
        auto hash = std::abs(Util::MurmurHash64A(f.first));
        if ((hash % max_threads) != thread_no) {
          continue;
        }

        syncd_total_cnt.fetch_add(1);

        const auto& file_src_path = dir_src_path + "/" + f.first;
        const auto& file_dst_path = dir_dst_path + "/" + f.first;

        if (!Util::Exists(file_src_path)) {
          LOG(ERROR) << file_src_path << " src file not exists when sync";
          success = false;
          syncd_fail_cnt.fetch_add(1);
          continue;
        }

        if (-1 == f.second.update_time()) {
          success = false;
          LOG(ERROR) << file_src_path << " invalid update time";
          syncd_fail_cnt.fetch_add(1);
          continue;
        }

        int64_t update_time = 0, file_size = 0;
        Util::FileInfo(file_dst_path, &update_time, &file_size);
        if (update_time != -1 && update_time == f.second.update_time() &&
            file_size == f.second.size()) {
          syncd_success_cnt.fetch_add(1);
          syncd_skipped_cnt.fetch_add(1);
          continue;
        }

        bool ret = true;
        if (f.second.file_type() == proto::Symlink) {
          ret = Util::SyncSymlink(ctx->src, ctx->dst, file_src_path);
        } else {
          ret = Util::CopyFile(file_src_path, file_dst_path);
        }

        if (!ret) {
          LOG(ERROR) << "Sync error: " << f.first;
          absl::base_internal::SpinLockHolder locker(&lock_);
          ctx->copy_failed_files.push_back(file_src_path);
          success = false;
          syncd_fail_cnt.fetch_add(1);
          continue;
        }

        ret = Util::SetUpdateTime(file_dst_path, f.second.update_time());
        if (!ret) {
          LOG(ERROR) << "Set update_time error: " << file_dst_path;
          absl::base_internal::SpinLockHolder locker(&lock_);
          ctx->copy_failed_files.push_back(file_src_path);
          success = false;
          syncd_fail_cnt.fetch_add(1);
          continue;
        }

        syncd_success_cnt.fetch_add(1);
      }
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
          LOG(ERROR) << "Delete " << dst_path << " error";
          continue;
        }
      }
    }
    return success;
  }

  void SyncStatusDir(ScanContext* ctx) {
    const auto& dst_path = ScanManager::Instance()->GenFileName(ctx->dst);
    ctx->status->set_path(ctx->dst);
    ctx->src = ctx->dst;
    ScanManager::Instance()->Dump(ctx);
  }

  void ProgressTask(bool* stop_progress_task, ScanContext* ctx) {
    while (!(*stop_progress_task)) {
      std::unique_lock<std::mutex> lock(mu_);
      if (cond_var_.wait_for(
              lock, std::chrono::seconds(10),
              [stop_progress_task] { return *stop_progress_task; })) {
        break;
      }
      LOG(INFO) << "Total: "
                << ctx->status->file_num() + ctx->status->symlink_num()
                << ", syncd count: " << syncd_total_cnt
                << ", success count: " << syncd_success_cnt
                << ", skipped count: " << syncd_skipped_cnt
                << ", failed count: " << syncd_fail_cnt;
    }
    LOG(INFO) << "ProgressTask Exists";
  }

 private:
  mutable absl::base_internal::SpinLock lock_;
  const int max_threads = 4;

  std::mutex mu_;
  std::condition_variable cond_var_;
  bool stop_progress_task_ = false;
  std::atomic<int64_t> syncd_total_cnt = 0;
  std::atomic<int64_t> syncd_success_cnt = 0;
  std::atomic<int64_t> syncd_fail_cnt = 0;
  std::atomic<int64_t> syncd_skipped_cnt = 0;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
