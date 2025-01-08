/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "folly/Singleton.h"
#include "src/util/sqlite_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class FileProcessManager final {
 private:
  friend class folly::Singleton<FileProcessManager>;
  FileProcessManager() {}

 public:
  static std::shared_ptr<FileProcessManager> Instance();

  ~FileProcessManager() {
    for (auto& task : process_tasks_) {
      if (task.joinable()) {
        task.join();
      }
    }
  }

  bool Init() {
    for (int i = 0; i < kTaskNum; ++i) {
      std::unordered_map<std::string, common::ReceiveContext> q;
      q.reserve(1000);
      queues_.emplace_back(q);
      auto task = std::thread(std::bind(&FileProcessManager::Process, this, i));
      process_tasks_.emplace_back(task);
    }
    return true;
  }

  void Stop() {
    stop_.store(true);
    for (auto& cv : cvs_) {
      cv.notify_all();
    }
    while (!Stopped()) {
      util::Util::Sleep(500);
    }
  }

  bool Stopped() {
    for (auto stopped : process_task_exits) {
      if (!stopped) {
        return false;
      }
    }
    return true;
  }

  void Put(const proto::FileReq& req) {
    const uint32_t thread_no =
        std::abs(util::Util::XXHash(req.request_id())) % kTaskNum;
    auto& q = queues_[thread_no];
    {
      absl::base_internal::SpinLockHolder locker(&locks_[thread_no]);
      auto it = q.find(req.request_id());
      if (it != q.end()) {
        it->second.partitions.insert(req.cur_part());
      } else {
        common::ReceiveContext ctx;
        ctx.repo_type = req.repo_type();
        ctx.repo_uuid = req.repo_uuid();
        ctx.repo_dir = req.repo_dir();
        ctx.total_part_num = util::Util::FilePartitionNum(
            req.file().file_size(), req.size_per_part());
        ctx.partitions.insert(req.cur_part());
        ctx.file = std::move(req.file());
        q.emplace(req.request_id(), ctx);
      }
      cvs_[thread_no].notify_all();
    }
    LOG(INFO) << "Queue size: " << q.size();
  }

  bool WriteFileMeta(const common::ReceiveContext& /*ctx*/) { return true; }

  bool WriteDB(const common::ReceiveContext& /*ctx*/) { return true; }

  bool GenThumbnail(const common::ReceiveContext& ctx) {
    auto repo_file_path =
        util::Util::RepoFilePath(ctx.repo_location, ctx.file.file_hash());
    std::string content;
    if (!util::Util::ResizeImg(repo_file_path, &content)) {
      return false;
    }
    std::string blake3_hash;
    if (!util::Util::Blake3(content, &blake3_hash)) {
      return false;
    }

    repo_file_path = util::Util::RepoFilePath(ctx.repo_location, blake3_hash);
    if (util::Util::WriteToFile(repo_file_path, content)) {
      return false;
    }
    *const_cast<common::ReceiveContext&>(ctx).file.mutable_thumb_hash() =
        blake3_hash;

    return true;
  }

  bool WriteMeta(const common::ReceiveContext& ctx) {
    if (ctx.file.file_type() == proto::FileType::Regular &&
        ctx.file.file_sub_type() == proto::FileSubType::FST_Photo) {
      if (!GenThumbnail(ctx)) {
        return false;
      }
    }

    std::string json;
    if (util::Util::MessageToConciseJson(ctx.file, &json)) {
      return false;
    }
    auto meta_file_path =
        util::Util::RepoFilePath(ctx.repo_location, ctx.file.file_hash()) +
        ".json";
    if (util::Util::WriteToFile(meta_file_path, json)) {
      return false;
    }

    return true;
  }

  bool DoProcess(const common::ReceiveContext& ctx) {
    if (ctx.total_part_num == ctx.partitions.size()) {
      if (WriteMeta(ctx)) {
        return true;
      }
    }
    return false;
  }

  void DoProcess(const int thread_no) {
    auto& q = queues_[thread_no];
    absl::base_internal::SpinLockHolder locker(&locks_[thread_no]);
    for (auto it = q.begin(); it != q.end();) {
      if (DoProcess(it->second)) {
        it = q.erase(it);
        continue;
      }
      ++it;
    }
  }

  void Process(const int thread_no) {
    LOG(INFO) << "Process task running";

    auto& q = queues_[thread_no];
    while (true) {
      if (stop_.load()) {
        break;
      }

      if (q.empty()) {
        std::unique_lock<std::mutex> lock(mus_[thread_no]);
        cvs_[thread_no].wait_for(lock, std::chrono::seconds(60));
      }

      DoProcess(thread_no);
    }

    DoProcess(thread_no);

    process_task_exits[thread_no] = true;
    LOG(INFO) << "Process task exists";
  }

 private:
  static const int32_t kTaskNum = 4;

  std::atomic<bool> stop_ = false;
  bool process_task_exits[kTaskNum] = {false};
  std::vector<absl::base_internal::SpinLock> locks_;
  std::vector<std::mutex> mus_;
  std::vector<std::condition_variable> cvs_;
  std::vector<std::thread> process_tasks_;
  std::vector<std::unordered_map<std::string, common::ReceiveContext>> queues_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_FILE_PROCESS_MANAGER_H
