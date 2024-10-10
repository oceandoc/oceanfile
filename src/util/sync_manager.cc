/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/sync_manager.h"

#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/client/grpc_client/grpc_file_client.h"

namespace oceandoc {
namespace util {

static folly::Singleton<SyncManager> sync_manager;

std::shared_ptr<SyncManager> SyncManager::Instance() {
  return sync_manager.try_get();
}

int32_t SyncManager::SyncRemote(SyncContext* sync_ctx) {
  auto ret = ValidateParameters(sync_ctx);
  if (ret) {
    return ret;
  }

  sync_ctx->Reset();

  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;
  std::shared_ptr<grpc::Channel> channel_(
      grpc::CreateChannel(sync_ctx->remote_addr + ":" + sync_ctx->remote_port,
                          grpc::InsecureChannelCredentials()));

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

  ret = ScanManager::Instance()->ParallelScan(&scan_ctx);
  if (ret) {
    LOG(ERROR) << "Scan " << sync_ctx->src << " error";
    return ret;
  }

  bool success = true;
  auto progress_task = std::bind(&SyncManager::ProgressTask, this, sync_ctx);
  ThreadPool::Instance()->Post(progress_task);

  LOG(INFO) << "Now sync " << scan_ctx.src << " to " << scan_ctx.dst;
  LOG(INFO) << "Memory usage: " << Util::MemUsage() << "MB";

  std::unordered_set<std::string> copy_failed_files;
  std::vector<std::future<bool>> rets;

  for (int i = 0; i < sync_ctx->max_threads; ++i) {
    std::packaged_task<bool()> task(
        std::bind(&SyncManager::RemoteSyncWorker, this, i, sync_ctx));
    rets.emplace_back(task.get_future());
    ThreadPool::Instance()->Post(task);
  }
  for (auto& f : rets) {
    if (f.get() == false) {
      success = false;
    }
  }

  Print(sync_ctx);
  sync_ctx->stop_progress_task = true;
  sync_ctx->cond_var.notify_all();
  SyncStatusDir(&scan_ctx);

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

bool SyncManager::RemoteSyncWorker(const int32_t thread_no,
                                   SyncContext* sync_ctx) {
  client::FileClient file_client(sync_ctx->remote_addr, sync_ctx->remote_port,
                                 proto::RepoType::RT_Remote,
                                 common::NET_BUFFER_SIZE_BYTES, false);

  for (const auto& d : sync_ctx->scan_ctx->status->scanned_dirs()) {
    auto hash = std::abs(Util::MurmurHash64A(d.first));
    if ((hash % sync_ctx->max_threads) != thread_no) {
      continue;
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

    client::SendContext send_ctx;
    send_ctx.src = dir_src_path;
    send_ctx.dst = dir_dst_path;
    send_ctx.type = proto::FileType::Dir;

    if (!file_client.Send(send_ctx)) {
      LOG(ERROR) << "Send " << send_ctx.src << " error";
    }

    for (const auto& f : d.second.files()) {
      const auto& file_src_path = dir_src_path + "/" + f.first;
      const auto& file_dst_path = dir_dst_path + "/" + f.first;

      if (!Util::Exists(file_src_path)) {
        LOG(ERROR) << file_src_path << " not exists";
        continue;
      }

      send_ctx.src = file_src_path;
      send_ctx.dst = file_src_path;
      send_ctx.type = f.second.file_type();

      if (!file_client.Send(send_ctx)) {
        LOG(ERROR) << "Send " << send_ctx.src << " error";
      }
    }
  }
  return true;
}

}  // namespace util
}  // namespace oceandoc
