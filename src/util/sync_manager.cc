/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sync_manager.h"

#include <filesystem>

#include "src/client/grpc_client/grpc_file_client.h"

namespace oceandoc {
namespace util {

static folly::Singleton<SyncManager> sync_manager;

std::shared_ptr<SyncManager> SyncManager::Instance() {
  return sync_manager.try_get();
}

int32_t SyncManager::WriteToFile(const proto::FileReq& req) {
  static thread_local std::shared_mutex mu;

  if (req.file_type() == proto::FileType::Dir) {
    if (!Util::Mkdir(req.dst())) {
      return Err_Path_mkdir_error;
    }
    return Err_Success;
  } else if (req.file_type() == proto::FileType::Symlink) {
    try {
      std::filesystem::create_symlink(req.content(), req.dst());
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << e.what();
      return Err_Path_create_symlink_error;
    }
    return Err_Success;
  }

  Util::MkParentDir(req.dst());

  auto err_code = Err_Success;
  err_code = Util::CreateFileWithSize(req.dst(), req.size());
  if (err_code != Err_Success) {
    LOG(ERROR) << "Create file error: " << req.dst();
    return err_code;
  }

  int64_t start = 0, end = 0;
  Util::CalcPartitionStart(req.size(), req.partition_num(),
                           req.partition_size(), &start, &end);
  if (end - start + 1 != static_cast<int64_t>(req.content().size())) {
    LOG(ERROR) << "Calc size error, partition_num: " << req.partition_num()
               << ", start: " << start << ", end: " << end
               << ", content size: " << req.content().size();
    return Err_File_partition_size_error;
  }
  LOG(INFO) << "Now store file " << req.dst()
            << ", total part: " << (req.size() / req.partition_size() + 1)
            << ", part: " << req.partition_num() + 1;
  std::unique_lock<std::shared_mutex> locker(mu);
  return Util::WriteToFile(req.dst(), req.content(), start);
}

int32_t SyncManager::SyncRemote(SyncContext* sync_ctx) {
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
  sync_ctx->Reset();

  proto::ScanStatus scan_status;
  scan_status.set_path(sync_ctx->src);
  ScanContext scan_ctx;
  scan_ctx.src = sync_ctx->src;
  scan_ctx.dst = sync_ctx->src;
  scan_ctx.status = &scan_status;
  scan_ctx.hash_method = sync_ctx->hash_method;
  scan_ctx.sync_method = sync_ctx->sync_method;
  scan_ctx.disable_scan_cache = sync_ctx->disable_scan_cache;
  scan_ctx.skip_scan = sync_ctx->skip_scan;
  sync_ctx->scan_ctx = &scan_ctx;

  auto ret = ScanManager::Instance()->ParallelScan(&scan_ctx);
  if (ret) {
    LOG(ERROR) << "Scan " << sync_ctx->src << " error";
    return ret;
  }

  bool success = true;
  auto progress_task = std::bind(&SyncManager::ProgressTask, this, sync_ctx);
  ThreadPool::Instance()->Post(progress_task);

  LOG(INFO) << "Now sync " << scan_ctx.src << " to " << scan_ctx.dst;
  LOG(INFO) << "Memory usage: " << Util::MemUsage() << "MB";
  syncing_.fetch_add(1);

  std::unordered_set<std::string> copy_failed_files;
  std::vector<std::future<void>> rets;
  for (int i = 0; i < sync_ctx->max_threads; ++i) {
    std::packaged_task<void()> task(
        std::bind(&SyncManager::RemoteSyncWorker, this, i, sync_ctx));
    rets.emplace_back(task.get_future());
    ThreadPool::Instance()->Post(task);
  }

  for (auto& f : rets) {
    f.get();
  }

  Print(sync_ctx);
  sync_ctx->stop_progress_task = true;
  sync_ctx->cond_var.notify_all();
  SyncStatusDir(&scan_ctx);
  syncing_.fetch_sub(1);

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

void SyncManager::RemoteSyncWorker(const int32_t thread_no,
                                   SyncContext* sync_ctx) {
  client::FileClient file_client(
      sync_ctx->remote_addr, sync_ctx->remote_port, proto::RepoType::RT_Remote,
      common::NET_BUFFER_SIZE_BYTES, sync_ctx->hash_method);
  file_client.Init();

  for (const auto& d : sync_ctx->scan_ctx->status->scanned_dirs()) {
    auto hash = std::abs(Util::MurmurHash64A(d.first));
    if ((hash % sync_ctx->max_threads) != thread_no) {
      continue;
    }

    while (file_client.Size() > 1000) {
      Util::Sleep(1000);
    }

    const std::string& dir_src_path = sync_ctx->src + "/" + d.first;
    const std::string& dir_dst_path = sync_ctx->dst + "/" + d.first;

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

    client::SendContext* dir_send_ctx = new client::SendContext();
    dir_send_ctx->src = dir_src_path;
    dir_send_ctx->dst = dir_dst_path;
    dir_send_ctx->type = proto::FileType::Dir;
    dir_send_ctx->op = proto::FileOp::FileExists;
    file_client.Put(dir_send_ctx);

    for (const auto& f : d.second.files()) {
      const auto& file_src_path = dir_src_path + "/" + f.first;
      const auto& file_dst_path = dir_dst_path + "/" + f.first;

      if (!Util::Exists(file_src_path)) {
        LOG(ERROR) << file_src_path << " not exists";
        continue;
      }

      client::SendContext* file_send_ctx = new client::SendContext();
      file_send_ctx->src = file_src_path;
      file_send_ctx->dst = file_dst_path;
      file_send_ctx->type = f.second.file_type();
      file_send_ctx->op = proto::FileOp::FileExists;
      if (f.second.file_type() == proto::FileType::Symlink) {
        if (!Util::SyncRemoteSymlink(dir_src_path, file_src_path,
                                     &file_send_ctx->content)) {
          LOG(ERROR) << "Get " << file_dst_path << " target error";
          continue;
        }
      }

      file_client.Put(file_send_ctx);
    }
  }
  file_client.Await();
}

}  // namespace util
}  // namespace oceandoc
