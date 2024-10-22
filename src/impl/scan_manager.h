/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_SCAN_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_SCAN_MANAGER_H

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/blocking_queue.h"
#include "src/common/defs.h"
#include "src/common/error.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class ScanManager {
 private:
  friend class folly::Singleton<ScanManager>;
  ScanManager() = default;

 public:
  static std::shared_ptr<ScanManager> Instance();

  bool Init() { return true; }

  std::string GenFileName(const std::string& path) {
    return path + "/" + common::CONFIG_DIR + "/" + util::Util::SHA256(path);
  }

  bool LoadCachedScanStatus(common::ScanContext* ctx) {
    const std::string& cached_status_path = GenFileName(ctx->src);
    if (!util::Util::Exists(cached_status_path)) {
      LOG(INFO) << cached_status_path << " not exists";
      return false;
    }

    std::string content, decompressed_content;
    if (util::Util::LoadSmallFile(cached_status_path, &content)) {
      if (!util::Util::LZMADecompress(content, &decompressed_content)) {
        LOG(ERROR) << "Decomppress error: " << cached_status_path;
        return false;
      }
      if (!ctx->status->ParseFromString(decompressed_content)) {
        LOG(ERROR) << "Parse error: " << cached_status_path;
        return false;
      }

      for (const auto& dir : ctx->ignored_dirs) {
        ctx->status->mutable_ignored_dirs()->insert({dir, true});
      }

      LOG(INFO) << "Load cache status success: " << Print(*ctx);
      return true;
    }
    LOG(ERROR) << "Load cache status error: " << cached_status_path;
    return false;
  }

  bool ValidateScanStatus(common::ScanContext* ctx) {
    const auto& path = ctx->src;
    if (!util::Util::Exists(path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }

    if (!LoadCachedScanStatus(ctx)) {
      return false;
    }
    const auto& status = *ctx->status;

    if (status.path() != path) {
      LOG(ERROR) << "Does " << path << " moved from " << status.path();
      return false;
    }

    int64_t file_num = 0;
    int64_t symlink_num = 0;
    for (const auto& p : status.scanned_dirs()) {
      if (p.first != p.second.path()) {
        LOG(ERROR) << "This should never happend";
        return false;
      }

      if (util::Util::IsAbsolute(p.first)) {
        LOG(ERROR) << "Path should be relative: " << p.first;
        return false;
      }

      const auto& file_items = p.second.files();
      for (const auto& file : file_items) {
        if (file.second.file_type() == proto::FileType::Regular) {
          ++file_num;
        } else {
          ++symlink_num;
        }
      }
    }

    if (file_num != status.file_num() || symlink_num != status.symlink_num()) {
      LOG(ERROR) << "file num or symlink num inconsistent: status file num: "
                 << status.file_num() << ", statistic file num: " << file_num
                 << ", status symlink num: " << status.symlink_num()
                 << ", statistic symlink num: " << symlink_num;
      return false;
    }

    LOG(INFO) << "Validate success, " << Print(*ctx);
    return true;
  }

  void SumStatus(common::ScanContext* ctx) {
    int64_t file_num = 0;
    int64_t symlink_num = 0;
    for (const auto& p : ctx->status->scanned_dirs()) {
      for (const auto& t : p.second.files()) {
        if (t.second.file_type() == proto::FileType::Regular) {
          ++file_num;
        } else {
          ++symlink_num;
        }
      }
    }
    ctx->status->set_file_num(file_num);
    ctx->status->set_symlink_num(symlink_num);
  }

  int32_t Dump(common::ScanContext* ctx) {
    std::string path = GenFileName(ctx->src);
    {
      absl::base_internal::SpinLockHolder locker(&ctx->lock);
      if (ctx->status->uuid().empty()) {
        ctx->status->set_uuid(util::Util::UUID());
      }
    }

    bool cp_ret = true;
    if (util::Util::Exists(path)) {
      cp_ret = util::Util::CopyFile(path, path + ".tmp");
    }

    std::string content, compressed_content;
    {
      absl::base_internal::SpinLockHolder locker(&ctx->lock);
      SumStatus(ctx);
      if (!ctx->status->SerializeToString(&content)) {
        LOG(ERROR) << "Serialize error";
        return Err_Serialize_error;
      }
    }
    util::Util::LZMACompress(content, &compressed_content);
    if (util::Util::WriteToFile(path, compressed_content, false) ==
            Err_Success ||
        util::Util::WriteToFile(path, compressed_content, false) ==
            Err_Success) {
      LOG(INFO) << "Dump success: " << Print(*ctx);
      return Err_Success;
    }
    LOG(ERROR) << "Dump failed";
    if (cp_ret) {
      util::Util::CopyFile(path + ".tmp", path);
    }
    return Err_Scan_dump_error;
  }

  void DumpTask(common::ScanContext* ctx) {
    int64_t last_time = util::Util::CurrentTimeMillis();
    while (!(ctx->stop_dump_task)) {
      std::unique_lock<std::mutex> lock(ctx->mu);
      if (ctx->cond_var.wait_for(lock, std::chrono::seconds(10),
                                 [ctx] { return ctx->stop_dump_task; })) {
        break;
      }
      int64_t cur_time = util::Util::CurrentTimeMillis();
      if (cur_time - last_time > 10 * 60 * 1000) {
        Dump(ctx);
        last_time = util::Util::CurrentTimeMillis();
      }
      LOG(INFO) << "Resident memory usage: " << util::Util::MemUsage()
                << "MB, scanned dir num: " << ctx->scanned_dir_num
                << ", skip dir num: " << ctx->skip_dir_num;
    }
    LOG(INFO) << "DumpTask for " << ctx->src << " exists";
  }

  void Stop() {
    stop_.store(true);
    while (scanning_.load() > 0) {
      util::Util::Sleep(1000);
    }
  }

  std::string Print(const common::ScanContext& ctx) {
    std::stringstream sstream;
    absl::base_internal::SpinLockHolder locker(&ctx.lock);
    sstream << ctx.src << ", dir num: " << ctx.status->scanned_dirs().size()
            << ", file num: " << ctx.status->file_num()
            << ", symlink file num: " << ctx.status->symlink_num()
            << ", ignored dir num: " << ctx.status->ignored_dirs().size()
            << ", added file num: " << ctx.added_files.size()
            << ", removed file num: " << ctx.removed_files.size();
    return sstream.str();
  }

  int32_t AddDirItem(const std::string& path, common::ScanContext* ctx,
                     const std::string& relative_path) {
    proto::Dir dir;
    dir.set_path(relative_path);

    int64_t update_time = 0, size = 0;
    if (!util::Util::FileInfo(path, &update_time, &size, dir.mutable_user(),
                              dir.mutable_group())) {
      LOG(ERROR) << "FileInfo error: " << path;
      return Err_File_permission_or_not_exists;
    }
    dir.set_update_time(update_time);
    {
      absl::base_internal::SpinLockHolder locker(&ctx->lock);
      auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
      if (it != ctx->status->mutable_scanned_dirs()->end()) {
        it->second.Swap(&dir);
      } else {
        (*ctx->status->mutable_scanned_dirs())[relative_path] = dir;
        ctx->added_files.insert(path);
      }
    }
    return Err_Success;
  }

  int32_t CalcHash(const std::string& path, common::ScanContext* ctx,
                   proto::File* file) {
    if (ctx->hash_method == common::HashMethod::Hash_SHA256) {
      if (!util::Util::FileSHA256(path, file->mutable_file_hash())) {
        return Err_File_hash_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_BLAKE3) {
      if (!util::Util::FileMD5(path, file->mutable_file_hash())) {
        return Err_File_hash_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_MD5) {
      if (!util::Util::FileMD5(path, file->mutable_file_hash())) {
        return Err_File_hash_calc_error;
      }
    } else if (ctx->hash_method == common::HashMethod::Hash_CRC32) {
      if (!util::Util::FileMD5(path, file->mutable_file_hash())) {
        return Err_File_hash_calc_error;
      }
    }
    return Err_Success;
  }

  int32_t AddFileItem(const std::string& file_name,
                      const proto::FileType file_type, common::ScanContext* ctx,
                      const std::string& parent_relative_path) {
    const auto& path = ctx->src + "/" + parent_relative_path + "/" + file_name;
    proto::File file;
    file.set_file_name(file_name);
    int64_t update_time = 0, file_size = 0;
    if (!util::Util::FileInfo(path, &update_time, &file_size,
                              file.mutable_user(), file.mutable_group())) {
      LOG(ERROR) << "FileInfo error: " << path;
      return Err_File_permission_or_not_exists;
    }

    file.set_update_time(update_time);
    file.set_file_size(file_size);
    file.set_file_type(file_type);

    auto ret = CalcHash(path, ctx, &file);
    if (ret != Err_Success) {
      LOG(ERROR) << "CalcHash error: " << path;
      return ret;
    }

    {
      absl::base_internal::SpinLockHolder locker(&ctx->lock);
      auto dir_it =
          ctx->status->mutable_scanned_dirs()->find(parent_relative_path);
      if (dir_it == ctx->status->mutable_scanned_dirs()->end()) {
        LOG(ERROR) << "This should never happen, parent_path: "
                   << parent_relative_path;
        return Err_Fail;
      }
      auto file_it = dir_it->second.mutable_files()->find(file_name);
      if (file_it != dir_it->second.mutable_files()->end()) {
        file_it->second.Swap(&file);
      } else {
        (*dir_it->second.mutable_files())[file_name] = file;
        ctx->added_files.emplace(path);
      }
    }
    return Err_Success;
  }

  int32_t RemoveDir(common::ScanContext* ctx, const std::string& cur_dir,
                    const std::string& relative_path) {
    absl::base_internal::SpinLockHolder locker(&ctx->lock);
    ctx->removed_files.insert(cur_dir);
    auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
    if (it == ctx->status->mutable_scanned_dirs()->end()) {
      return Err_Success;
    }

    for (const auto& p : it->second.files()) {
      ctx->removed_files.emplace(cur_dir + "/" + p.first);
    }

    ctx->status->mutable_scanned_dirs()->erase(relative_path);
    return Err_Success;
  }

  int32_t RemoveFile(common::ScanContext* ctx, const std::string& cur_dir,
                     const std::set<std::string>& files,
                     const std::string& relative_path) {
    absl::base_internal::SpinLockHolder locker(&ctx->lock);
    auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
    if (it == ctx->status->mutable_scanned_dirs()->end()) {
      LOG(ERROR) << "This should never happen: " << cur_dir;
      return Err_Fail;
    }

    for (auto file_it = it->second.mutable_files()->begin();
         file_it != it->second.mutable_files()->end();) {
      if (files.find(file_it->first) == files.end()) {
        ctx->removed_files.emplace(cur_dir + "/" + file_it->first);
        file_it = it->second.mutable_files()->erase(file_it);
      } else {
        ++file_it;
      }
    }
    return Err_Success;
  }

  int32_t ParallelScan(common::ScanContext* ctx) {
    if (!util::Util::Exists(ctx->src)) {
      LOG(ERROR) << ctx->src << " not exists";
      return Err_File_not_exists;
    }

    if (!std::filesystem::is_directory(ctx->src)) {
      LOG(ERROR) << ctx->src << " not directory";
      return Err_File_not_dir;
    }

    if (scanning_.load() > 0) {
      LOG(ERROR) << "Another scan is running...";
      return Err_Scan_busy;
    }

    scanning_.fetch_add(1);
    LOG(INFO) << "Now scan " << ctx->src;

    bool load_cache_success = true;
    if (!ctx->disable_scan_cache) {
      load_cache_success = LoadCachedScanStatus(ctx);
    }

    if (load_cache_success && ctx->skip_scan) {
      return Err_Success;
    }

    ctx->dir_queue.PushBack(ctx->src);  // all elements is absolute path
    for (const auto& p : ctx->status->scanned_dirs()) {
      if (p.first.empty()) {
        continue;
      }
      ctx->dir_queue.PushBack(ctx->src + "/" + p.first);
    }

    auto dump_task = std::bind(&ScanManager::DumpTask, this, ctx);
    util::ThreadPool::Instance()->Post(dump_task);

    for (int32_t i = 0; i < ctx->max_threads; ++i) {
      ctx->running_mark.fetch_or(1ULL << i);
      auto task = std::bind(&ScanManager::ParallelFullScan, this, i, ctx);
      util::ThreadPool::Instance()->Post(task);
    }

    while (ctx->running_mark) {
      for (int32_t i = 0; i < ctx->max_threads; ++i) {
        if (ctx->running_mark & (1ULL << i)) {
          continue;
        }
        util::Util::Sleep(1000);
        if (ctx->dir_queue.Size() <= 0) {
          break;
        }
        ctx->running_mark.fetch_or(1ULL << i);
        auto task = std::bind(&ScanManager::ParallelFullScan, this, i, ctx);
        util::ThreadPool::Instance()->Post(task);
      }
      util::Util::Sleep(1000);
    }

    ctx->stop_dump_task = true;
    ctx->cond_var.notify_all();

    if (ctx->err_code != Err_Success) {
      LOG(ERROR) << "Scan " << ctx->src << " has error: " << ctx->err_code;
    }

    if (ctx->err_code == Err_Success) {
      ctx->status->set_complete_time(util::Util::CurrentTimeMillis());
      ctx->status->set_hash_method((int32_t)ctx->hash_method);
    }

    if (Dump(ctx) != Err_Success) {
      ctx->err_code = Err_Scan_dump_error;
    }

    scanning_.fetch_sub(1);
    return ctx->err_code;
  }

  void ParallelFullScan(const int32_t thread_no, common::ScanContext* ctx) {
    LOG(INFO) << "Thread " << thread_no << " for scan " << ctx->src
              << " running";
    static thread_local std::atomic<int32_t> count = 0;
    while (true) {
      if (stop_.load()) {
        ctx->err_code = Err_Scan_interrupted;
        break;
      }

      std::string cur_dir;
      int try_times = 0;
      while (try_times < 3 && !ctx->dir_queue.PopBack(&cur_dir)) {
        util::Util::Sleep(100);
        ++try_times;
      }

      if (try_times >= 3) {
        break;
      }

      std::string relative_path;
      util::Util::Relative(cur_dir, ctx->src, &relative_path);
      ctx->scanned_dir_num.fetch_add(1);
      auto it = ctx->status->ignored_dirs().find(relative_path);
      if (it != ctx->status->ignored_dirs().end()) {
        continue;
      }

      if (!util::Util::Exists(cur_dir)) {
        RemoveDir(ctx, cur_dir, relative_path);
        continue;
      }

      auto update_time = util::Util::UpdateTime(cur_dir);
      if (update_time == -1) {
        LOG(INFO) << "Scan error: " << cur_dir;
        ctx->err_code = Err_File_permission_or_not_exists;
        continue;
      }

      {
        absl::base_internal::SpinLockHolder locker(&ctx->lock);
        auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
        if (it != ctx->status->mutable_scanned_dirs()->end()) {
          if (it->second.update_time() == update_time &&
              ctx->hash_method ==
                  common::HashMethod(ctx->status->hash_method())) {
            ctx->skip_dir_num.fetch_add(1);
            continue;
          }
        }
      }

      auto ret = AddDirItem(cur_dir, ctx, relative_path);
      if (ret != Err_Success) {
        ctx->err_code = ret;
        continue;
      }

      ret = Err_Success;
      try {
        std::set<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
          const auto& filename = entry.path().filename().string();
          files.insert(filename);
          if (entry.is_symlink()) {
            ret |= AddFileItem(filename, proto::FileType::Symlink, ctx,
                               relative_path);
          } else if (entry.is_regular_file()) {
            ret |= AddFileItem(filename, proto::FileType::Regular, ctx,
                               relative_path);
          } else if (entry.is_directory()) {
            ctx->dir_queue.PushBack(entry.path().string());
          } else {
            LOG(ERROR) << "Unknow file type: " << entry.path();
          }
        }

        if (ret != Err_Success) {
          ctx->err_code = ret;
          continue;
        }

        RemoveFile(ctx, cur_dir, files, relative_path);

        {
          absl::base_internal::SpinLockHolder locker(&ctx->lock);
          auto it = ctx->status->mutable_scanned_dirs()->find(relative_path);
          if (it != ctx->status->mutable_scanned_dirs()->end()) {
            it->second.set_update_time(update_time);
          }
        }

      } catch (const std::filesystem::filesystem_error& e) {
        LOG(ERROR) << "Scan error: " << cur_dir << ", exception: " << e.what();
      }

      if ((count % 100) == 0) {
        LOG(INFO) << "Scanning: " << cur_dir << ", thread_no: " << thread_no;
      }
      ++count;
    }
    ctx->running_mark.fetch_and(~(1ULL << thread_no));
    LOG(INFO) << "Thread " << thread_no << " for " << ctx->src << " exist";
  }

 private:
  std::atomic<uint32_t> scanning_ = 0;
  std::atomic<bool> stop_ = false;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_SCAN_MANAGER_H
