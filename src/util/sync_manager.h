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

  void Stop() {
    stop_.store(true);
    ScanManager::Instance()->Stop();
    while (syncing_.load() > 0) {
      Util::Sleep(1000);
    }
  }

  int32_t WriteToFile(const proto::FileReq& req);

 private:
  void SyncStatusDir(common::ScanContext* scan_ctx) {
    const auto& dst_path = ScanManager::Instance()->GenFileName(scan_ctx->dst);
    scan_ctx->status->set_path(scan_ctx->dst);
    scan_ctx->src = scan_ctx->dst;
    ScanManager::Instance()->Dump(scan_ctx);
  }

  void Print(common::SyncContext* sync_ctx) {
    if (sync_ctx->scan_ctx) {
      LOG(INFO) << "Total dir num: " << sync_ctx->total_dir_cnt
                << " total file num: " << sync_ctx->total_file_cnt
                << ", syncd file success num: "
                << sync_ctx->syncd_file_success_cnt
                << ", syncd file fail num: " << sync_ctx->syncd_file_fail_cnt
                << ", syncd file skip num: "
                << sync_ctx->syncd_file_skipped_cnt;
    } else {
      LOG(INFO) << "syncd file success num: "
                << sync_ctx->syncd_file_success_cnt
                << ", syncd file fail num: " << sync_ctx->syncd_file_fail_cnt
                << ", syncd file skip num: "
                << sync_ctx->syncd_file_skipped_cnt;
    }
  }

  void RecursiveProgressTask(common::SyncContext* sync_ctx) {
    while (!sync_ctx->stop_progress_task) {
      std::unique_lock<std::mutex> lock(sync_ctx->mu);
      if (sync_ctx->cond_var.wait_for(
              lock, std::chrono::seconds(10),
              [sync_ctx] { return sync_ctx->stop_progress_task; })) {
        break;
      }
      Print(sync_ctx);
    }
    LOG(INFO) << "RecursiveProgressTask Exists";
  }

  void ProgressTask(common::SyncContext* sync_ctx) {
    while (!sync_ctx->stop_progress_task) {
      std::unique_lock<std::mutex> lock(sync_ctx->mu);
      if (sync_ctx->cond_var.wait_for(
              lock, std::chrono::seconds(10),
              [sync_ctx] { return sync_ctx->stop_progress_task; })) {
        break;
      }
      Print(sync_ctx);
    }
    LOG(INFO) << "ProgressTask Exists";
  }

  int32_t ValidateLocalSyncParameters(common::SyncContext* sync_ctx) {
    if (!Util::IsAbsolute(sync_ctx->src) || !Util::IsAbsolute(sync_ctx->dst)) {
      LOG(ERROR) << "Path must be absolute";
      return Err_Path_not_absolute;
    }

    Util::UnifyDir(&sync_ctx->src);
    Util::UnifyDir(&sync_ctx->dst);

    if (Util::StartWith(sync_ctx->src, sync_ctx->dst)) {
      LOG(ERROR) << "Cannot sync " << sync_ctx->src << " to " << sync_ctx->dst
                 << ", for cannot sync to subdir";
      return Err_Path_dst_is_src_subdir;
    }

    if (!Util::Exists(sync_ctx->src) || !Util::Exists(sync_ctx->dst)) {
      LOG(ERROR) << "Src or dst not exists";
      return Err_Path_not_exists;
    }

    if (!std::filesystem::is_directory(sync_ctx->src) ||
        !std::filesystem::is_directory(sync_ctx->dst)) {
      LOG(ERROR) << "Src or dst not dir";
      return Err_Path_not_dir;
    }
    return Err_Success;
  }

  int32_t ValidateRemoteSyncParameters(common::SyncContext* sync_ctx) {
    if (!Util::IsAbsolute(sync_ctx->src) || !Util::IsAbsolute(sync_ctx->dst)) {
      LOG(ERROR) << "Path must be absolute";
      return Err_Path_not_absolute;
    }

    Util::UnifyDir(&sync_ctx->src);
    Util::UnifyDir(&sync_ctx->dst);

    if (!Util::Exists(sync_ctx->src)) {
      LOG(ERROR) << "Src or dst not exists";
      return Err_Path_not_exists;
    }

    if (!std::filesystem::is_directory(sync_ctx->src)) {
      LOG(ERROR) << "Src or dst not dir";
      return Err_Path_not_dir;
    }
    return Err_Success;
  }

 public:
  int32_t SyncLocalRecursive(common::SyncContext* sync_ctx) {
    auto ret = ValidateLocalSyncParameters(sync_ctx);
    if (ret) {
      return ret;
    }

    sync_ctx->Reset();
    LOG(INFO) << "Now sync local " << sync_ctx->src << " to " << sync_ctx->dst;
    syncing_.fetch_add(1);

    auto progress_task =
        std::bind(&SyncManager::RecursiveProgressTask, this, sync_ctx);
    ThreadPool::Instance()->Post(progress_task);

    sync_ctx->dir_queue.PushBack(sync_ctx->src);
    for (int32_t i = 0; i < sync_ctx->max_threads; ++i) {
      sync_ctx->running_mark.fetch_or(1ULL << i);
      auto task =
          std::bind(&SyncManager::RecursiveLocalSyncWorker, this, i, sync_ctx);
      ThreadPool::Instance()->Post(task);
    }

    while (sync_ctx->running_mark) {
      for (int32_t i = 0; i < sync_ctx->max_threads; ++i) {
        if (sync_ctx->running_mark & (1ULL << i)) {
          continue;
        }

        Util::Sleep(1000);
        if (sync_ctx->dir_queue.Size() <= 0) {
          continue;
        }

        sync_ctx->running_mark.fetch_or(1ULL << i);
        auto task = std::bind(&SyncManager::RecursiveLocalSyncWorker, this, i,
                              sync_ctx);
        ThreadPool::Instance()->Post(task);
      }
      Util::Sleep(1000);
    }

    sync_ctx->stop_progress_task = true;
    sync_ctx->cond_var.notify_all();

    for (const auto& file : sync_ctx->sync_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }

    Print(sync_ctx);
    syncing_.fetch_sub(1);
    if (sync_ctx->err_code) {
      LOG(INFO) << "Sync success";
      return Err_Success;
    }

    LOG(ERROR) << "Sync exists error";
    return sync_ctx->err_code;
  }

  int32_t SyncLocal(common::SyncContext* sync_ctx) {
    auto ret = ValidateLocalSyncParameters(sync_ctx);
    if (ret) {
      return ret;
    }

    sync_ctx->Reset();
    syncing_.fetch_add(1);
    auto progress_task = std::bind(&SyncManager::ProgressTask, this, sync_ctx);
    ThreadPool::Instance()->Post(progress_task);

    proto::ScanStatus scan_status;
    scan_status.set_path(sync_ctx->src);

    common::ScanContext scan_ctx;
    scan_ctx.src = sync_ctx->src;
    scan_ctx.dst = sync_ctx->src;
    scan_ctx.status = &scan_status;
    scan_ctx.hash_method = sync_ctx->hash_method;
    scan_ctx.sync_method = sync_ctx->sync_method;
    scan_ctx.disable_scan_cache = sync_ctx->disable_scan_cache;
    scan_ctx.skip_scan = sync_ctx->skip_scan;
    sync_ctx->scan_ctx = &scan_ctx;
    scan_ctx.ignored_dirs.insert(sync_ctx->ignored_dirs.begin(),
                                 sync_ctx->ignored_dirs.end());

    ret = ScanManager::Instance()->ParallelScan(&scan_ctx);
    if (ret) {
      LOG(ERROR) << "Scan " << sync_ctx->src << " error";
      return ret;
    }

    sync_ctx->total_dir_cnt = scan_status.scanned_dirs_size();
    sync_ctx->total_file_cnt =
        scan_status.file_num() + scan_status.symlink_num();

    LOG(INFO) << "Now sync local " << sync_ctx->src << " to " << sync_ctx->dst;
    LOG(INFO) << "Memory usage: " << Util::MemUsage() << "MB";

    std::vector<std::future<void>> rets;
    for (int i = 0; i < sync_ctx->max_threads; ++i) {
      std::packaged_task<void()> task(
          std::bind(&SyncManager::LocalSyncWorker, this, i, sync_ctx));
      rets.emplace_back(task.get_future());
      ThreadPool::Instance()->Post(task);
    }
    for (auto& f : rets) {
      f.get();
    }

    sync_ctx->stop_progress_task = true;
    sync_ctx->cond_var.notify_all();

    for (const auto& file : sync_ctx->sync_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }

    Print(sync_ctx);
    syncing_.fetch_sub(1);

    if (sync_ctx->err_code == Err_Success) {
      LOG(INFO) << "Sync success";
      return Err_Success;
    }

    LOG(ERROR) << "Sync exists error";
    return sync_ctx->err_code;
  }

  int32_t SyncRemote(common::SyncContext* sync_ctx);
  int32_t SyncRemoteRecursive(common::SyncContext* sync_ctx);

 private:
  void RecursiveLocalSyncWorker(const int thread_no,
                                common::SyncContext* sync_ctx) {
    LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
              << " running";
    while (true) {
      if (stop_.load()) {
        sync_ctx->err_code = Err_Scan_interrupted;
        break;
      }

      std::string cur_dir;
      int try_times = 0;
      while (try_times < 3 && !sync_ctx->dir_queue.PopBack(&cur_dir)) {
        Util::Sleep(100);
        ++try_times;
      }

      if (try_times >= 3) {
        break;
      }

      std::string relative_path;
      Util::Relative(cur_dir, sync_ctx->src, &relative_path);
      auto it = sync_ctx->ignored_dirs.find(relative_path);
      if (it != sync_ctx->ignored_dirs.end()) {
        continue;
      }

      if (!Util::Exists(cur_dir)) {
        continue;
      }

      sync_ctx->total_dir_cnt.fetch_add(1);
      const auto& dst_dir = sync_ctx->dst + "/" + relative_path;
      Util::Mkdir(dst_dir);
      try {
        for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
          if (entry.is_directory() && !entry.is_symlink()) {
            sync_ctx->dir_queue.PushBack(entry.path().string());
            continue;
          }

          sync_ctx->total_file_cnt.fetch_add(1);
          bool ret = true;
          const auto& filename = entry.path().filename().string();
          const auto& file_src_path = cur_dir + "/" + filename;
          const auto& file_dst_path =
              sync_ctx->dst + "/" + relative_path + "/" + filename;

          int64_t src_update_time = 0, src_size = 0;
          std::string user, group;
          if (!Util::FileInfo(file_src_path, &src_update_time, &src_size, &user,
                              &group)) {
            LOG(ERROR) << "FileInfo error: " << file_src_path;
            sync_ctx->err_code = Err_File_permission_or_not_exists;
          }

          if (Util::Exists(file_dst_path)) {
            int64_t dst_update_time = 0, dst_size = 0;
            if (!Util::FileInfo(file_dst_path, &dst_update_time, &dst_size,
                                nullptr, nullptr)) {
              LOG(ERROR) << "FileInfo error: " << file_dst_path;
              sync_ctx->err_code = Err_File_permission_or_not_exists;
            }

            if (src_update_time != -1 && src_update_time == dst_update_time &&
                src_size == dst_size) {
              sync_ctx->syncd_file_success_cnt.fetch_add(1);
              sync_ctx->syncd_file_skipped_cnt.fetch_add(1);
              continue;
            }
          }

          if (entry.is_symlink()) {
            ret =
                Util::SyncSymlink(sync_ctx->src, sync_ctx->dst, file_src_path);
          } else if (entry.is_regular_file()) {
            ret = Util::CopyFile(file_src_path, file_dst_path);
          }

          if (!ret) {
            LOG(ERROR) << "Sync error: " << file_src_path;
            absl::base_internal::SpinLockHolder locker(&sync_ctx->lock);
            sync_ctx->sync_failed_files.push_back(file_src_path);
            sync_ctx->syncd_file_fail_cnt.fetch_add(1);
            continue;
          }

          ret = Util::SetUpdateTime(file_dst_path, src_update_time);
          if (!ret) {
            LOG(ERROR) << "Set update_time error: " << file_dst_path;
            absl::base_internal::SpinLockHolder locker(&sync_ctx->lock);
            sync_ctx->sync_failed_files.push_back(file_src_path);
            sync_ctx->syncd_file_fail_cnt.fetch_add(1);
            continue;
          }
          sync_ctx->syncd_file_success_cnt.fetch_add(1);
        }
      } catch (const std::filesystem::filesystem_error& e) {
        LOG(ERROR) << "Scan error: " << cur_dir << ", exception: " << e.what();
      }
    }
    sync_ctx->running_mark.fetch_and(~(1ULL << thread_no));
    LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
              << " exist";
  }

  void LocalSyncWorker(const int thread_no, common::SyncContext* sync_ctx) {
    for (const auto& d : sync_ctx->scan_ctx->status->scanned_dirs()) {
      if (stop_.load()) {
        sync_ctx->err_code = Err_Sync_interrupted;
        break;
      }
      auto hash = std::abs(Util::MurmurHash64A(d.first));
      if ((hash % sync_ctx->max_threads) != thread_no) {
        continue;
      }

      const auto& dir_src_path = sync_ctx->src + "/" + d.first;
      const auto& dir_dst_path = sync_ctx->dst + "/" + d.first;

      if (!Util::Exists(dir_src_path)) {
        LOG(ERROR) << dir_src_path << " not exists";
        continue;
      }

      if (!std::filesystem::is_directory(dir_src_path)) {
        LOG(ERROR) << dir_src_path << " not dir";
        continue;
      }

      if (std::filesystem::is_symlink(dir_src_path)) {
        LOG(ERROR) << dir_src_path << " is symlink";
      }

      if (!Util::Exists(dir_dst_path)) {
        Util::Mkdir(dir_dst_path);
      }

      if (!d.first.empty()) {
        if (!Util::SetUpdateTime(dir_dst_path, d.second.update_time())) {
          LOG(ERROR) << "Set update time error: " << dir_dst_path;
        }
      }

      for (const auto& f : d.second.files()) {
        const auto& file_src_path = dir_src_path + "/" + f.first;
        const auto& file_dst_path = dir_dst_path + "/" + f.first;

        if (!Util::Exists(file_src_path)) {
          LOG(ERROR) << file_src_path << " src file not exists when sync";
          sync_ctx->err_code = Err_File_permission_or_not_exists;
          sync_ctx->syncd_file_fail_cnt.fetch_add(1);
          continue;
        }

        if (-1 == f.second.update_time()) {
          LOG(ERROR) << file_src_path << " invalid update time";
          sync_ctx->err_code = Err_File_permission_or_not_exists;
          sync_ctx->syncd_file_fail_cnt.fetch_add(1);
          continue;
        }

        int64_t update_time = 0, file_size = 0;
        Util::FileInfo(file_dst_path, &update_time, &file_size, nullptr,
                       nullptr);
        if (update_time != -1 && update_time == f.second.update_time() &&
            file_size == f.second.size()) {
          sync_ctx->syncd_file_success_cnt.fetch_add(1);
          sync_ctx->syncd_file_skipped_cnt.fetch_add(1);
          continue;
        }

        bool ret = true;
        if (f.second.file_type() == proto::Symlink) {
          ret = Util::SyncSymlink(sync_ctx->src, sync_ctx->dst, file_src_path);
        } else {
          ret = Util::CopyFile(file_src_path, file_dst_path);
        }

        if (!ret) {
          LOG(ERROR) << "Sync error: " << f.first;
          sync_ctx->err_code = Err_File_copy_error;
          absl::base_internal::SpinLockHolder locker(&sync_ctx->lock);
          sync_ctx->sync_failed_files.push_back(file_src_path);
          sync_ctx->syncd_file_fail_cnt.fetch_add(1);
          continue;
        }

        ret = Util::SetUpdateTime(file_dst_path, f.second.update_time());
        if (!ret) {
          LOG(ERROR) << "Set update_time error: " << file_dst_path;
          sync_ctx->err_code = Err_File_permission_or_not_exists;
          absl::base_internal::SpinLockHolder locker(&sync_ctx->lock);
          sync_ctx->sync_failed_files.push_back(file_src_path);
          sync_ctx->syncd_file_fail_cnt.fetch_add(1);
          continue;
        }

        sync_ctx->syncd_file_success_cnt.fetch_add(1);
      }
    }

    if (sync_ctx->sync_method == common::SyncMethod::Sync_SYNC) {
      for (const auto& p : sync_ctx->scan_ctx->removed_files) {
        auto hash = std::abs(Util::MurmurHash64A(p));
        if ((hash % sync_ctx->max_threads) != thread_no) {
          continue;
        }

        if (Util::IsAbsolute(p)) {
          LOG(ERROR) << "Must be relative: " << p;
          continue;
        }

        const auto& src_path = sync_ctx->src + "/" + p;
        const auto& dst_path = sync_ctx->dst + "/" + p;
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
  }

  void RecursiveRemoteSyncWorker(const int32_t thread_no,
                                 common::SyncContext* sync_ctx);

  void RemoteSyncWorker(const int32_t thread_no, common::SyncContext* sync_ctx);

 private:
  std::atomic<uint32_t> syncing_ = 0;
  std::atomic<bool> stop_ = false;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_SYNC_MANAGER_H
