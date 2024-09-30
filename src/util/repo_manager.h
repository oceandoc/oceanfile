/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/proto/data.pb.h"
#include "src/util/scan_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class RepoManager {
 private:
  friend class folly::Singleton<RepoManager>;
  RepoManager() = default;

 public:
  static std::shared_ptr<RepoManager> Instance();

  bool Init() {
    std::string content;
    auto ret = Util::LoadSmallFile(common::REPOS_CONFIG_FILE, &content);

    absl::base_internal::SpinLockHolder locker(&lock_);
    if (ret && !Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repos config error, content: " << content;
      return false;
    }

    for (const auto& p : repos_.repos()) {
      repos_.mutable_uuid_repos()->insert({p.second.uuid(), p.second});
    }

    CheckRepos();
    return true;
  }

  // TODO
  bool CheckRepos() { return true; }

  // TODO: copy to a tmp file first
  bool FlushReposConfig() {
    std::string content;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (!Util::MessageToJson(repos_, &content)) {
        LOG(ERROR) << "Repos config convert to json error";
        return false;
      }
    }

    if (Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ||
        Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ||
        Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false)) {
      LOG(INFO) << "Flush repos config success";
      return true;
    }
    LOG(ERROR) << "Disaster: flush repos config error";
    return false;
  }

  std::string RepoDir(const std::string& path) {
    return path + "/" + common::CONFIG_DIR;
  }

  std::string RepoPath(const std::string& path, const std::string& uuid) {
    return path + "/" + common::CONFIG_DIR + "/" + uuid + ".repo";
  }

  bool ExistsRepo(const std::string& path, std::string* uuid = nullptr) {
    std::string repo_config_dir = path + "/" + common::CONFIG_DIR;
    if (!Util::Exists(repo_config_dir) ||
        !std::filesystem::is_directory(repo_config_dir)) {
      return false;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(repo_config_dir)) {
      if (entry.is_symlink() || !entry.is_directory()) {
        continue;
      }
      if (entry.path().extension().string() == ".repo") {
        if (uuid) {
          *uuid = entry.path().filename().string();
        }
        return true;
      }
    }
    return false;
  }

  bool RestoreRepo(const std::string& path, const std::string& uuid) {
    LOG(INFO) << "Now restore repo from path: " << path;
    std::string content;
    proto::Repo repo;
    std::string repo_path = RepoPath(path, uuid);

    if (!Util::LoadSmallFile(repo_path, &content)) {
      return false;
    }

    if (!Util::JsonToMessage(content, &repo)) {
      LOG(ERROR) << "Parse repo config error, content: " << content;
      return false;
    }

    if (path != repo.path()) {
      LOG(WARNING) << "Repo moved, origin path: " << repo.path();
      repo.set_path(path);
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      repos_.mutable_repos()->insert({repo.path(), repo});
      repos_.mutable_uuid_repos()->insert({repo.uuid(), repo});
    }
    return FlushReposConfig();
  }

  bool RestoreRepo(const std::string& path) {
    std::string uuid;
    if (ExistsRepo(path, &uuid)) {
      return RestoreRepo(path, uuid);
    }
    return false;
  }

  bool CreateRepo(const std::string& path, std::string* uuid) {
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = repos_.repos().find(path);
      if (it != repos_.repos().end()) {
        *uuid = it->second.uuid();
        return true;
      }
    }

    if (ExistsRepo(path, uuid)) {
      RestoreRepo(path, *uuid);
      return true;
    }

    *uuid = Util::UUID();
    const std::string& repo_file_path = RepoPath(path, *uuid);
    proto::Repo repo;
    repo.set_create_time(Util::ToTimeStr(Util::CurrentTimeMillis()));
    repo.set_uuid(*uuid);
    repo.set_path(path);

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      repos_.mutable_repos()->insert({path, repo});
      repos_.mutable_uuid_repos()->insert({*uuid, repo});
    }

    if (!FlushReposConfig()) {
      return false;
    }

    std::string content;
    if (!Util::MessageToJson(repo, &content)) {
      LOG(ERROR) << "Convert to json error";
      return false;
    }
    return Util::WriteToFile(repo_file_path, content, false);
  }

  bool DeleteRepoByPath(const std::string& path) {
    LOG(INFO) << "Now delete repo from path: " << path;
    std::string uuid;
    if (!ExistsRepo(path, &uuid)) {
      return true;
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = repos_.mutable_repos()->find(path);
      if (it != repos_.repos().end()) {
        repos_.mutable_repos()->erase(it);
      }

      it = repos_.mutable_uuid_repos()->find(uuid);
      if (it != repos_.uuid_repos().end()) {
        repos_.mutable_uuid_repos()->erase(it);
      }
    }

    Util::Remove(RepoDir(path));
    return true;
  }

  bool DeleteRepoByUUID(const std::string& uuid) {
    LOG(INFO) << "Now delete repo from uuid: " << uuid;
    auto it = repos_.uuid_repos().find(uuid);
    if (it != repos_.uuid_repos().end()) {
      return DeleteRepoByPath(it->second.path());
    }
    return true;
  }

  std::string RepoPathByUUID(const std::string& uuid) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repos_.uuid_repos().find(uuid);
    if (it != repos_.uuid_repos().end()) {
      return it->second.path();
    }
    return "";
  }

  bool WriteToFile(const std::string& repo_uuid, const std::string& sha256,
                   const std::string& content, const int64_t size,
                   const int32_t partition_num, const int64_t partition_size) {
    static thread_local std::shared_mutex mu;
    const auto& repo_path = RepoPathByUUID(repo_uuid);
    if (repo_path.empty()) {
      LOG(ERROR) << "Invalid repo path";
      return false;
    }

    const auto& repo_file_path = Util::RepoFilePath(repo_path, sha256);
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return false;
    }
    LOG(INFO) << "Now store file " << repo_file_path
              << ", part: " << partition_num;

    Util::MkParentDir(repo_file_path);

    if (!Util::CreateFileWithSize(repo_file_path, size)) {
      LOG(ERROR) << "Create file error: " << repo_file_path;
      return false;
    }

    int64_t start = 0, end = 0;
    Util::CalcPartitionStart(size, partition_num, partition_size, &start, &end);
    if (end - start + 1 != static_cast<int64_t>(content.size())) {
      LOG(ERROR) << "Calc size error, partition_num: " << partition_num
                 << ", start: " << start << ", end: " << end
                 << ", content size: " << content.size();
      return false;
    }

    std::unique_lock<std::shared_mutex> locker(mu);
    return Util::WriteToFile(repo_file_path, content, start);
  }

  void SyncStatusDir(const std::string& canonical_src,
                     const std::string& canonical_dst) {
    const auto& src = ScanManager::Instance()->GenFileDir(canonical_src);
    const auto& dst = canonical_dst + "/" + common::CONFIG_DIR;
    Util::Copy(src, dst);
  }

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
    for (int i = 0; i < max_thread; ++i) {
      std::packaged_task<bool()> task(
          std::bind(&RepoManager::SyncWorker, this, i, &scan_status,
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

  void Stop() {
    stop_.store(true);
    ScanManager::Instance()->Stop();
  }

 private:
  bool SyncWorker(const int no, proto::ScanStatus* status,
                  const std::string& src, const std::string& dst,
                  std::unordered_set<std::string>* copy_failed_files) {
    bool success = true;
    for (int i = no; i < status->scanned_files().size(); i += max_thread) {
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

    for (int i = no; i < status->scanned_dirs().size(); i += max_thread) {
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

 private:
  proto::Repos repos_;
  const int max_thread = 5;
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::shared_mutex mutex_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
