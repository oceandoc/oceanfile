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

    return true;
  }

  // TODO(xiedeacc): copy to a tmp file first
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
    proto::RepoMeta repo;
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
    proto::RepoMeta repo;
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

  proto::ErrCode WriteToFile(const std::string& repo_uuid,
                             const std::string& sha256,
                             const std::string& content, const int64_t size,
                             const int32_t partition_num,
                             const int64_t partition_size) {
    static thread_local std::shared_mutex mu;
    const auto& repo_path = RepoPathByUUID(repo_uuid);
    if (repo_path.empty()) {
      LOG(ERROR) << "Invalid repo path";
      return proto::ErrCode::Repo_not_exists;
    }

    const auto& repo_file_path = Util::RepoFilePath(repo_path, sha256);
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return proto::ErrCode::Repo_uuid_error;
    }

    Util::MkParentDir(repo_file_path);

    auto err_code = proto::ErrCode::Success;
    err_code = Util::CreateFileWithSize(repo_file_path, size);
    if (err_code != proto::ErrCode::Success) {
      LOG(ERROR) << "Create file error: " << repo_file_path;
      return err_code;
    }

    int64_t start = 0, end = 0;
    Util::CalcPartitionStart(size, partition_num, partition_size, &start, &end);
    if (end - start + 1 != static_cast<int64_t>(content.size())) {
      LOG(ERROR) << "Calc size error, partition_num: " << partition_num
                 << ", start: " << start << ", end: " << end
                 << ", content size: " << content.size();
      return proto::ErrCode::File_partition_size_error;
    }
    LOG(INFO) << "Now store file " << repo_file_path
              << ", part: " << partition_num;
    std::unique_lock<std::shared_mutex> locker(mu);
    return Util::WriteToFile(repo_file_path, content, start);
  }

  void Stop() { stop_.store(true); }

 private:
  proto::Repos repos_;
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::shared_mutex mutex_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
