/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/sync_manager.h"

#include <filesystem>
#include <memory>

#include "src/client/grpc_client/grpc_file_client.h"
#include "src/common/defs.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<SyncManager> sync_manager;

std::shared_ptr<SyncManager> SyncManager::Instance() {
  return sync_manager.try_get();
}

int32_t SyncManager::WriteToFile(const proto::FileReq& req) {
  static thread_local std::shared_mutex mu;

  if (req.file_type() == proto::FileType::Direcotry) {
    if (!util::Util::Mkdir(req.dst())) {
      return Err_File_mkdir_error;
    }
    return Err_Success;
  } else if (req.file_type() == proto::FileType::Symlink) {
    try {
      std::filesystem::create_symlink(req.content(), req.dst());
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << e.what();
      return Err_File_create_symlink_error;
    }
    return Err_Success;
  }

  util::Util::MkParentDir(req.dst());

  auto err_code = Err_Success;
  err_code = util::Util::CreateFileWithSize(req.dst(), req.file_size());
  if (err_code != Err_Success) {
    LOG(ERROR) << "Create file error: " << req.dst();
    return err_code;
  }

  int64_t start = 0, end = 0;
  util::Util::CalcPartitionStart(req.file_size(), req.partition_num(),
                                 req.partition_size(), &start, &end);
  if (end - start + 1 != static_cast<int64_t>(req.content().size())) {
    LOG(ERROR) << "Calc size error, partition_num: " << req.partition_num()
               << ", start: " << start << ", end: " << end
               << ", content size: " << req.content().size();
    return Err_File_partition_size_error;
  }
  std::unique_lock<std::shared_mutex> locker(mu);
  return util::Util::WriteToFile(req.dst(), req.content(), start);
}

int32_t SyncManager::SyncRemoteRecursive(common::SyncContext* sync_ctx) {
  auto ret = ValidateRemoteSyncParameters(sync_ctx);
  if (ret) {
    sync_ctx->err_code = ret;
    return ret;
  }

  sync_ctx->Reset();

  LOG(INFO) << "Now sync remote " << sync_ctx->src
            << " host: " << sync_ctx->remote_addr << ", dir: " << sync_ctx->dst;

  syncing_.fetch_add(1);
  client::FileClient file_client(sync_ctx);
  file_client.Start();

  sync_ctx->dir_queue.PushBack(sync_ctx->src);
  for (int32_t i = 0; i < sync_ctx->max_threads; ++i) {
    sync_ctx->running_mark.fetch_or(1ULL << i);
    auto task =
        std::bind(&SyncManager::RecursiveRemoteSyncWorker, this, i, sync_ctx);
    util::ThreadPool::Instance()->Post(task);
  }

  while (sync_ctx->running_mark) {
    for (int32_t i = 0; i < sync_ctx->max_threads; ++i) {
      if (sync_ctx->running_mark & (1ULL << i)) {
        continue;
      }

      util::Util::Sleep(1000);
      if (sync_ctx->dir_queue.Size() <= 0) {
        continue;
      }

      sync_ctx->running_mark.fetch_or(1ULL << i);
      auto task =
          std::bind(&SyncManager::RecursiveLocalSyncWorker, this, i, sync_ctx);
      util::ThreadPool::Instance()->Post(task);
    }
    util::Util::Sleep(1000);
  }

  file_client.Await();
  syncing_.fetch_sub(1);

  if (sync_ctx->err_code == Err_Success) {
    LOG(INFO) << "Sync success";
    return Err_Success;
  }

  LOG(INFO) << "Sync exists error";
  return Err_Fail;
}

void SyncManager::RecursiveRemoteSyncWorker(const int32_t thread_no,
                                            common::SyncContext* sync_ctx) {
  LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
            << " running";
  client::FileClient file_client(sync_ctx);
  file_client.Start();
  while (true) {
    if (stop_.load()) {
      sync_ctx->err_code = Err_Sync_interrupted;
      break;
    }

    std::string cur_dir;
    int try_times = 0;
    while (try_times < 3 && !sync_ctx->dir_queue.PopBack(&cur_dir)) {
      util::Util::Sleep(100);
      ++try_times;
    }

    if (try_times >= 3) {
      break;
    }

    std::string relative_path;
    util::Util::Relative(cur_dir, sync_ctx->src, &relative_path);
    auto it = sync_ctx->ignored_dirs.find(relative_path);
    if (it != sync_ctx->ignored_dirs.end()) {
      continue;
    }

    if (!util::Util::Exists(cur_dir)) {
      continue;
    }

    while (file_client.Size() > 1000) {
      util::Util::Sleep(1000);
    }

    sync_ctx->total_dir_cnt.fetch_add(1);
    const auto& dst_dir = sync_ctx->dst + "/" + relative_path;
    try {
      for (const auto& entry : std::filesystem::directory_iterator(cur_dir)) {
        if (entry.is_directory() && !entry.is_symlink()) {
          sync_ctx->dir_queue.PushBack(entry.path().string());
          std::shared_ptr<common::SendContext> dir_send_ctx =
              std::make_shared<common::SendContext>();
          dir_send_ctx->src = cur_dir;
          dir_send_ctx->dst = dst_dir;
          dir_send_ctx->file_type = proto::FileType::Direcotry;
          dir_send_ctx->op = proto::FileOp::FileExists;
          file_client.Put(dir_send_ctx);
          continue;
        }

        sync_ctx->total_file_cnt.fetch_add(1);
        const auto& filename = entry.path().filename().string();
        const auto& file_src_path = cur_dir + "/" + filename;
        const auto& file_dst_path =
            sync_ctx->dst + "/" + relative_path + "/" + filename;

        std::shared_ptr<common::SendContext> file_send_ctx =
            std::make_shared<common::SendContext>();
        file_send_ctx->src = file_src_path;
        file_send_ctx->dst = file_dst_path;
        file_send_ctx->op = proto::FileOp::FileExists;
        if (entry.is_symlink()) {
          file_send_ctx->file_type = proto::FileType::Symlink;
          if (!util::Util::SyncRemoteSymlink(cur_dir, file_src_path,
                                             &file_send_ctx->content)) {
            LOG(ERROR) << "Get " << file_dst_path << " target error";
            continue;
          }
        } else if (entry.is_regular_file()) {
          file_send_ctx->file_type = proto::FileType::Regular;
        }

        file_client.Put(file_send_ctx);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan error: " << cur_dir << ", exception: " << e.what();
    }
  }
  file_client.SetFillQueueComplete();
  file_client.Await();
  sync_ctx->running_mark.fetch_and(~(1ULL << thread_no));
  LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
            << " exist";
}

int32_t SyncManager::SyncRemote(common::SyncContext* sync_ctx) {
  auto ret = ValidateRemoteSyncParameters(sync_ctx);
  if (ret) {
    sync_ctx->err_code = ret;
    return ret;
  }

  sync_ctx->Reset();

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

  ret = ScanManager::Instance()->ParallelScan(&scan_ctx);
  if (ret) {
    LOG(ERROR) << "Scan " << sync_ctx->src << " error";
    return ret;
  }

  sync_ctx->total_dir_cnt = scan_status.scanned_dirs_size();
  sync_ctx->total_file_cnt = scan_status.file_num() + scan_status.symlink_num();

  LOG(INFO) << "Now sync remote " << sync_ctx->src
            << " host: " << sync_ctx->remote_addr << ", dir: " << sync_ctx->dst;
  LOG(INFO) << "Memory usage: " << util::Util::MemUsage() << "MB";
  syncing_.fetch_add(1);

  std::vector<std::future<void>> rets;
  for (int32_t i = 0; i < sync_ctx->max_threads; ++i) {
    std::packaged_task<void()> task(
        std::bind(&SyncManager::RemoteSyncWorker, this, i, sync_ctx));
    rets.emplace_back(task.get_future());
    util::ThreadPool::Instance()->Post(task);
  }

  for (auto& f : rets) {
    f.get();
  }

  sync_ctx->cond_var.notify_all();
  syncing_.fetch_sub(1);

  if (sync_ctx->err_code == Err_Success) {
    LOG(INFO) << "Sync success";
    return Err_Success;
  }

  LOG(ERROR) << "Sync exists error";
  return sync_ctx->err_code;
}

void SyncManager::RemoteSyncWorker(const int32_t thread_no,
                                   common::SyncContext* sync_ctx) {
  LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
            << " running";
  client::FileClient file_client(sync_ctx);
  file_client.Start();
  for (const auto& d : sync_ctx->scan_ctx->status->scanned_dirs()) {
    if (stop_.load()) {
      sync_ctx->err_code = Err_Sync_interrupted;
      break;
    }

    auto hash = std::abs(util::Util::MurmurHash64A(d.first));
    if ((hash % sync_ctx->max_threads) != thread_no) {
      continue;
    }

    while (file_client.Size() > 1000) {
      util::Util::Sleep(1000);
    }

    const std::string& dir_src_path = sync_ctx->src + "/" + d.first;
    const std::string& dir_dst_path = sync_ctx->dst + "/" + d.first;

    if (!util::Util::Exists(dir_src_path)) {
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

    std::shared_ptr<common::SendContext> dir_send_ctx =
        std::make_shared<common::SendContext>();
    dir_send_ctx->src = dir_src_path;
    dir_send_ctx->dst = dir_dst_path;
    dir_send_ctx->file_type = proto::FileType::Direcotry;
    dir_send_ctx->op = proto::FileOp::FileExists;
    file_client.Put(dir_send_ctx);

    for (const auto& f : d.second.files()) {
      const auto& file_src_path = dir_src_path + "/" + f.first;
      const auto& file_dst_path = dir_dst_path + "/" + f.first;

      if (!util::Util::Exists(file_src_path)) {
        LOG(ERROR) << file_src_path << " not exists";
        continue;
      }

      std::shared_ptr<common::SendContext> file_send_ctx =
          std::make_shared<common::SendContext>();
      file_send_ctx->src = file_src_path;
      file_send_ctx->dst = file_dst_path;
      file_send_ctx->file_type = f.second.file_type();
      file_send_ctx->op = proto::FileOp::FileExists;
      if (f.second.file_type() == proto::FileType::Symlink) {
        if (!util::Util::SyncRemoteSymlink(dir_src_path, file_src_path,
                                           &file_send_ctx->content)) {
          LOG(ERROR) << "Get " << file_dst_path << " target error";
          continue;
        }
      }

      file_client.Put(file_send_ctx);
    }
  }
  file_client.SetFillQueueComplete();
  file_client.Await();
  LOG(INFO) << "Thread " << thread_no << " for sync " << sync_ctx->src
            << " exist";
}

}  // namespace impl
}  // namespace oceandoc
