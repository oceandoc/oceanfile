/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/common/error.h"
#include "src/proto/data.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class RepoManager {
 private:
  friend class folly::Singleton<RepoManager>;
  RepoManager() = default;

 public:
  static std::shared_ptr<RepoManager> Instance();

  bool Init() {
    std::string content;
    auto ret = util::Util::LoadSmallFile(common::REPOS_CONFIG_FILE, &content);

    absl::base_internal::SpinLockHolder locker(&lock_);
    if (ret && !util::Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repos config error, content: " << content;
      return false;
    }
    return true;
  }

  bool FlushReposConfig() {
    auto ret = util::Util::CopyFile(common::REPOS_CONFIG_FILE,
                                    common::REPOS_CONFIG_TMP_FILE);
    if (!ret) {
      return false;
    }

    std::string content;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (!util::Util::MessageToJson(repos_, &content)) {
        LOG(ERROR) << "Repos config convert to json error";
        return false;
      }
    }

    if (util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ||
        util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ||
        util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false)) {
      LOG(INFO) << "Flush repos config success";
      return true;
    }
    util::Util::CopyFile(common::REPOS_CONFIG_TMP_FILE,
                         common::REPOS_CONFIG_FILE);
    LOG(ERROR) << "Disaster: flush repos config error";
    return false;
  }

  bool ExistsRepo(const std::string& path, std::string* uuid = nullptr) {
    std::string repo_config_dir = path + "/" + common::CONFIG_DIR;
    if (!util::Util::Exists(repo_config_dir) ||
        std::filesystem::is_symlink(repo_config_dir) ||
        !std::filesystem::is_directory(repo_config_dir)) {
      return false;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(repo_config_dir)) {
      if (entry.is_symlink() || entry.is_directory()) {
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
    std::string repo_config_file_path =
        path + "/" + common::CONFIG_DIR + "/" + uuid + ".repo";

    if (!util::Util::LoadSmallFile(repo_config_file_path, &content)) {
      return false;
    }

    if (!util::Util::JsonToMessage(content, &repo)) {
      LOG(ERROR) << "Parse repo config error, content: " << content;
      return false;
    }

    if (path != repo.path()) {
      LOG(WARNING) << "Repo moved, origin path: " << repo.path();
      repo.set_path(path);
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      (*repos_.mutable_repos())[repo.uuid()] = repo;
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

  bool CreateRepo(const std::string& name, const std::string& path,
                  std::string* uuid) {
    if (ExistsRepo(path, uuid)) {
      RestoreRepo(path, *uuid);
      return true;
    }

    *uuid = util::Util::UUID();
    std::string repo_config_file_path =
        path + "/" + common::CONFIG_DIR + "/" + *uuid + ".repo";

    proto::RepoMeta repo;
    repo.set_create_time(
        util::Util::ToTimeStr(util::Util::CurrentTimeMillis()));
    repo.set_uuid(*uuid);
    repo.set_path(path);
    repo.set_name(name);
    repo.set_location_uuid(util::Util::UUID());

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      repos_.mutable_repos()->insert({*uuid, repo});
    }

    if (!FlushReposConfig()) {
      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        repos_.mutable_repos()->erase(*uuid);
      }
      return false;
    }

    std::string content;
    if (!util::Util::MessageToJson(repo, &content)) {
      LOG(ERROR) << "Convert to json error";
      return false;
    }
    return util::Util::WriteToFile(repo_config_file_path, content, false);
  }

  bool DeleteRepoByUUID(const std::string& uuid) {
    LOG(INFO) << "Now delete repo from uuid: " << uuid;
    auto it = repos_.repos().find(uuid);
    if (it != repos_.repos().end()) {
      std::string uuid;
      if (!ExistsRepo(it->second.path(), &uuid)) {
        {
          absl::base_internal::SpinLockHolder locker(&lock_);
          repos_.mutable_repos()->erase(uuid);
        }
        return true;
      }

      std::string repo_config_file_path =
          it->second.path() + "/" + common::CONFIG_DIR + "/" + uuid + ".repo";
      if (!util::Util::Remove(repo_config_file_path)) {
        return false;
      }

      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        repos_.mutable_repos()->erase(uuid);
      }
    }
    return true;
  }

  std::string RepoPathByUUID(const std::string& uuid) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repos_.repos().find(uuid);
    if (it != repos_.repos().end()) {
      return it->second.path();
    }
    return "";
  }

  int32_t WriteToFile(const proto::FileReq& req) {
    static thread_local std::shared_mutex mu;
    const auto& repo_path = RepoPathByUUID(req.repo_uuid());
    if (repo_path.empty()) {
      LOG(ERROR) << "Invalid repo path";
      return Err_Repo_not_exists;
    }

    const auto& repo_file_path =
        util::Util::RepoFilePath(repo_path, req.hash());
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return Err_Repo_uuid_error;
    }

    util::Util::MkParentDir(repo_file_path);

    auto err_code = Err_Success;
    err_code = util::Util::CreateFileWithSize(repo_file_path, req.size());
    if (err_code != Err_Success) {
      LOG(ERROR) << "Create file error: " << repo_file_path;
      return err_code;
    }

    int64_t start = 0, end = 0;
    util::Util::CalcPartitionStart(req.size(), req.partition_num(),
                                   req.partition_size(), &start, &end);
    if (end - start + 1 != static_cast<int64_t>(req.content().size())) {
      LOG(ERROR) << "Calc size error, partition_num: " << req.partition_num()
                 << ", start: " << start << ", end: " << end
                 << ", content size: " << req.content().size();
      return Err_File_partition_size_error;
    }
    std::unique_lock<std::shared_mutex> locker(mu);
    return util::Util::WriteToFile(repo_file_path, req.content(), start);
  }

  int32_t ListServerDir(const proto::RepoReq& req, proto::RepoRes* res) {
    std::string path = req.path();
    if (path.empty()) {
      path = "/";
    }

    res->mutable_dir()->set_path(path);
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const auto& filename = entry.path().filename().string();

        proto::FileItem file;
        file.set_filename(filename);
        if (entry.is_symlink()) {
          file.set_file_type(proto::FileType::Symlink);
        } else if (entry.is_regular_file()) {
          file.set_file_type(proto::FileType::Regular);
        } else if (entry.is_directory()) {
          file.set_file_type(proto::FileType::Dir);
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
        res->mutable_dir()->mutable_files()->emplace(filename, file);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t ListRepoDir(const proto::RepoReq& req, proto::RepoRes* res) {
    if (req.repo_uuid().empty()) {
      return Err_Repo_uuid_error;
    }

    auto ret = GetDir(req, res);
    if (ret == Err_Success) {
      return Err_Success;
    }

    ret = LoadRepoData(req.repo_uuid());
    if (ret) {
      return ret;
    }

    ret = GetDir(req, res);
    if (ret) {
      return ret;
    }
    return Err_Success;
  }

  int32_t GetDir(const proto::RepoReq& req, proto::RepoRes* res) {
    std::string path = req.path();
    if (path.empty()) {
      path = "/";
    }

    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repo_datas_.find(req.repo_uuid());
    if (it != repo_datas_.end()) {
      auto dir_it = it->second.dirs().find(path);
      if (dir_it == it->second.dirs().end()) {
        LOG(ERROR) << "Cannot find dir " << path << " in repo: " << it->first
                   << ", path: " << it->second.path();
        return Err_File_not_exists;
      }
      *res->mutable_dir() = dir_it->second;
      return Err_Success;
    }
    return Err_File_not_exists;
  }

  int32_t LoadRepoData(const std::string& repo_uuid) {
    std::string repo_data_file_path = "./data/" + repo_uuid + ".repodata";
    proto::RepoData repo_data;
    if (util::Util::Exists(repo_data_file_path)) {
      auto ret = LoadRepoData(repo_data_file_path, &repo_data);
      if (ret) {
        return ret;
      }
    } else {
      const auto& repo_path = RepoPathByUUID(repo_uuid);
      if (repo_path.empty()) {
        LOG(ERROR) << "Invalid repo path";
        return Err_Repo_not_exists;
      }
      repo_data_file_path = "./data/" + repo_uuid + ".repodata";
      auto ret = LoadRepoData(repo_data_file_path, &repo_data);
      if (ret) {
        return ret;
      }
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    repo_datas_.emplace(repo_uuid, repo_data);
    return Err_Success;
  }

  int32_t LoadRepoData(const std::string& path, proto::RepoData* repo_data) {
    std::string content, decompressed_content;
    if (util::Util::LoadSmallFile(path, &content)) {
      if (!util::Util::LZMADecompress(content, &decompressed_content)) {
        LOG(ERROR) << "Decomppress error: " << path;
        return Err_Decompress_error;
      }

      if (!repo_data->ParseFromString(decompressed_content)) {
        LOG(ERROR) << "Parse error: " << path;
        return Err_Deserialize_error;
      }
      return Err_Success;
    }
    LOG(ERROR) << "Load cache status error: " << path;
    return Err_File_not_exists;
  }

  void Stop() { stop_.store(true); }

 private:
  proto::Repos repos_;
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::shared_mutex mutex_;
  std::map<std::string, proto::RepoData> repo_datas_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
