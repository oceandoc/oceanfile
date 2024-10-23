/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H

#include <chrono>
#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>

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

  ~RepoManager() {
    if (dump_task_.joinable()) {
      dump_task_.join();
    }
  }

  bool Init() {
    std::string content;
    auto ret = util::Util::LoadSmallFile(common::REPOS_CONFIG_FILE, &content);

    absl::base_internal::SpinLockHolder locker(&lock_);
    if (ret && !util::Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repos config error, content: " << content;
      return false;
    }
    dump_task_ = std::thread(&RepoManager::FlushRepoDataTask, this);
    LOG(INFO) << "Loaded repo num: " << repos_.repos().size();
    return true;
  }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
  }

  bool FlushRepoMeta() {
    bool copy_tmp = false;
    if (util::Util::Exists(common::REPOS_CONFIG_FILE)) {
      auto ret = util::Util::CopyFile(common::REPOS_CONFIG_FILE,
                                      common::REPOS_CONFIG_TMP_FILE);
      if (!ret) {
        return false;
      }
      copy_tmp = true;
    }

    std::string content;
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (!util::Util::MessageToJson(repos_, &content)) {
        LOG(ERROR) << "Repos config convert to json error";
        return false;
      }
    }

    if (util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ==
            Err_Success ||
        util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ==
            Err_Success ||
        util::Util::WriteToFile(common::REPOS_CONFIG_FILE, content, false) ==
            Err_Success) {
      LOG(INFO) << "Flush repos config success, num: " << repos_.repos_size();
      return true;
    }
    if (util::Util::Exists(common::REPOS_CONFIG_TMP_FILE) && copy_tmp) {
      util::Util::CopyFile(common::REPOS_CONFIG_TMP_FILE,
                           common::REPOS_CONFIG_FILE);
      LOG(ERROR) << "Disaster: flush repos config error";
    }
    return false;
  }

  void FlushRepoDataTask() {
    LOG(INFO) << "FlushRepoDataTask running";
    while (!stop_) {
      if (stop_) {
        break;
      }
      {
        std::unique_lock<std::mutex> locker(mu_);
        cv_.wait_for(locker, std::chrono::seconds(5),
                     [this] { return stop_.load(); });
      }

      absl::base_internal::SpinLockHolder locker(&lock_);
      for (const auto& p : repo_datas_) {
        if (ShouldFlushRepoData(p.first)) {
          FlushRepoData(p.second.repo_uuid());
        }
      }
    }
    LOG(INFO) << "FlushRepoDataTask exists";
  }

  bool ShouldFlushRepoData(const std::string /* repo_uuid */) { return true; }

  bool FlushRepoData(const std::string& repo_uuid) {
    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(repo_uuid, &repo_meta)) {
      LOG(ERROR) << "Cannot find repo meta: " << repo_uuid;
      return Err_Repo_meta_not_exists;
    }

    std::string repo_data_file_path = "./data/" + repo_uuid + ".data";
    bool copy_tmp = false;
    if (util::Util::Exists(repo_data_file_path)) {
      auto ret = util::Util::CopyFile(repo_data_file_path,
                                      repo_data_file_path + ".tmp");
      if (!ret) {
        return false;
      }
      copy_tmp = true;
    }

    std::string content;
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repo_datas_.find(repo_uuid);
    if (it != repo_datas_.end()) {
      if (!util::Util::MessageToJson(it->second, &content)) {
        LOG(ERROR) << "Repos data convert to json error";
        return false;
      }
      if (util::Util::WriteToFile(repo_data_file_path, content, false) ==
              Err_Success ||
          util::Util::WriteToFile(repo_data_file_path, content, false) ==
              Err_Success ||
          util::Util::WriteToFile(repo_data_file_path, content, false) ==
              Err_Success) {
        LOG(INFO) << "Flush repos data success, num: " << repo_uuid;
        return true;
      }
    } else {
      LOG(ERROR) << "Cannot find repo data: " << repo_uuid;
      return false;
    }

    if (util::Util::Exists(repo_data_file_path + ".tmp") && copy_tmp) {
      auto ret = util::Util::CopyFile(repo_data_file_path + ".tmp",
                                      repo_data_file_path);
      if (!ret) {
        return false;
      }
      copy_tmp = true;
    }

    return false;
  }

  int32_t ListUserRepo(const proto::RepoReq& req, proto::RepoRes* res) {
    if (repos_.repos().empty()) {
      return Err_Success;
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    for (const auto& p : repos_.repos()) {
      if (p.second.owner() != req.user()) {
        continue;
      }
      res->mutable_repos()->insert(p);
    }
    return Err_Success;
  }

  int32_t ListServerDir(const proto::RepoReq& req, proto::RepoRes* res) {
    std::string path = req.path();
    if (path.empty()) {
      path = "/";
    }

    res->mutable_dir()->set_path(path);
    try {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const auto& file_name = entry.path().filename().string();

        proto::File file;
        file.set_file_name(file_name);
        if (entry.is_symlink()) {
          file.set_file_type(proto::FileType::Symlink);
        } else if (entry.is_regular_file()) {
          file.set_file_type(proto::FileType::Regular);
        } else if (entry.is_directory()) {
          file.set_file_type(proto::FileType::Direcotry);
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
        res->mutable_dir()->mutable_files()->emplace(file_name, file);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t CreateServerDir(const proto::RepoReq& req, proto::RepoRes* res) {
    std::string path = req.path();
    if (path.empty()) {
      path = "/";
    }

    res->mutable_dir()->set_path(path);
    if (util::Util::Mkdir(path)) {
      return Err_Success;
    }
    return Err_File_mkdir_error;
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
          *uuid = entry.path().stem().string();
        }
        return true;
      }
    }
    return false;
  }

  bool RestoreRepo(const std::string& path) {
    proto::RepoMeta repo;
    if (ExistsRepo(path, repo.mutable_repo_uuid())) {
      return RestoreRepo(path, &repo);
    }
    return false;
  }

  bool RestoreRepo(const std::string& path, proto::RepoMeta* repo) {
    std::string content;
    std::string repo_config_file_path =
        path + "/" + common::CONFIG_DIR + "/" + repo->repo_uuid() + ".repo";
    LOG(INFO) << "Now restore repo from path: " << repo_config_file_path;

    if (!util::Util::LoadSmallFile(repo_config_file_path, &content)) {
      return false;
    }

    if (!util::Util::JsonToMessage(content, repo)) {
      LOG(ERROR) << "Parse repo config error, content: " << content;
      return false;
    }

    if (path != repo->repo_path()) {
      LOG(WARNING) << "Repo moved, origin path: " << repo->repo_path();
      repo->set_repo_path(path);
    }
    LOG(INFO) << "Restored " << repo->repo_uuid();

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (repos_.repos().find(repo->repo_uuid()) != repos_.repos().end()) {
        LOG(INFO) << "Repo already exists" << repo->repo_uuid();
        return Err_Success;
      }
      (*repos_.mutable_repos())[repo->repo_uuid()] = *repo;
    }
    return FlushRepoMeta();
  }

  int32_t CreateRepo(const proto::RepoReq& req, proto::RepoRes* res) {
    proto::RepoMeta repo;
    if (ExistsRepo(req.path(), repo.mutable_repo_uuid())) {
      if (RestoreRepo(req.path(), &repo)) {
        return Err_Success;
      }
      return Err_Repo_restore_repo_error;
    }

    repo.set_repo_uuid(util::Util::UUID());
    std::string repo_config_file_path = req.path() + "/" + common::CONFIG_DIR +
                                        "/" + repo.repo_uuid() + ".repo";

    repo.set_create_time(
        util::Util::ToTimeStr(util::Util::CurrentTimeMillis()));
    repo.set_update_time(repo.create_time());
    repo.set_repo_path(req.path());
    repo.set_repo_name(req.repo_name());
    repo.set_repo_location_uuid(util::Util::UUID());
    repo.set_owner(req.user());

    std::string content;
    if (!util::Util::MessageToJson(repo, &content)) {
      LOG(ERROR) << "Convert to json error";
      return Err_Repo_create_repo_error;
    }
    auto ret = util::Util::WriteToFile(repo_config_file_path, content, false);
    if (ret) {
      return ret;
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      repos_.mutable_repos()->insert({repo.repo_uuid(), repo});
    }

    if (!FlushRepoMeta()) {
      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        repos_.mutable_repos()->erase(repo.repo_uuid());
      }
      return Err_Repo_flush_repo_config_error;
    }

    *res->mutable_repo() = repo;
    return Err_Success;
  }

  int32_t DeleteRepo(const proto::RepoReq& req, proto::RepoRes* /*res*/) {
    LOG(INFO) << "Now delete repo: " << req.repo_uuid();
    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      auto it = repos_.repos().find(req.repo_uuid());
      if (it != repos_.repos().end()) {
        repos_.mutable_repos()->erase(req.repo_uuid());
      } else {
        return Err_Success;
      }
    }
    if (FlushRepoMeta()) {
      LOG(INFO) << "Repo deleted: " << req.repo_uuid();
      return Err_Success;
    }
    return Err_Repo_flush_repo_config_error;
  }

  bool RepoMetaByUUID(const std::string& uuid, proto::RepoMeta* repo_meta) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repos_.repos().find(uuid);
    if (it != repos_.repos().end()) {
      *repo_meta = it->second;
      return true;
    }
    return false;
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
    LOG(ERROR) << "Load cache repo data error: " << path;
    return Err_File_not_exists;
  }

  int32_t LoadRepoData(const std::string& repo_uuid) {
    std::string repo_data_file_path = "./data/" + repo_uuid + ".data";
    proto::RepoData repo_data;
    if (util::Util::Exists(repo_data_file_path)) {
      auto ret = LoadRepoData(repo_data_file_path, &repo_data);
      if (ret) {
        return ret;
      }
    } else {
      proto::RepoMeta repo_meta;
      if (!RepoMetaByUUID(repo_uuid, &repo_meta)) {
        LOG(ERROR) << "Invalid repo path";
        return Err_Repo_not_exists;
      }
      const auto& repo_path = repo_meta.repo_path();
      repo_data_file_path = repo_path + "/" + repo_uuid + ".data";
      auto ret = LoadRepoData(repo_data_file_path, &repo_data);
      if (ret) {
        return ret;
      }
    }
    absl::base_internal::SpinLockHolder locker(&lock_);
    repo_datas_.emplace(repo_uuid, repo_data);
    return Err_Success;
  }

  int32_t ListRepoDir(const proto::RepoReq& req, proto::RepoRes* res) {
    if (req.repo_uuid().empty()) {
      return Err_Repo_uuid_error;
    }

    auto ret = GetRepoDir(req, res);
    if (ret == Err_Success) {
      return Err_Success;
    }

    ret = LoadRepoData(req.repo_uuid());
    if (ret) {
      return ret;
    }

    ret = GetRepoDir(req, res);
    if (ret) {
      return ret;
    }
    return Err_Success;
  }

  int32_t GetRepoDir(const proto::RepoReq& req, proto::RepoRes* res) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repo_datas_.find(req.repo_uuid());
    if (it != repo_datas_.end()) {
      auto dir_it = it->second.dirs().find(req.path());
      if (dir_it == it->second.dirs().end()) {
        LOG(ERROR) << "Cannot find dir " << req.path()
                   << " in repo: " << it->first
                   << ", path: " << it->second.repo_path();
        return Err_File_not_exists;
      }
      *res->mutable_repo_dir() = dir_it->second;
      return Err_Success;
    }
    return Err_File_not_exists;
  }

  int32_t RepoFileExists(const proto::RepoReq& req, proto::RepoRes* /*res*/) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repo_datas_.find(req.repo_uuid());
    if (it != repo_datas_.end()) {
      auto dir_it = it->second.dirs().find(req.path());
      if (dir_it == it->second.dirs().end()) {
        LOG(ERROR) << "Cannot find dir " << req.path()
                   << " in repo: " << it->first
                   << ", path: " << it->second.repo_path();
        return Err_File_not_exists;
      }
      return Err_Success;
    }
    return Err_File_not_exists;
  }

  int32_t WriteToFile(const proto::FileReq& req) {
    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(req.repo_uuid(), &repo_meta)) {
      LOG(ERROR) << "Invalid repo path";
      return Err_Repo_not_exists;
    }
    const auto& repo_path = repo_meta.repo_path();
    const auto& repo_file_path =
        util::Util::RepoFilePath(repo_path, req.file_hash());
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return Err_Repo_uuid_error;
    }

    util::Util::MkParentDir(repo_file_path);

    auto err_code = Err_Success;
    err_code = util::Util::CreateFileWithSize(repo_file_path, req.file_size());
    if (err_code != Err_Success) {
      LOG(ERROR) << "Create file error: " << repo_file_path;
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

    static thread_local std::shared_mutex mu;
    std::unique_lock<std::shared_mutex> locker(mu);
    return util::Util::WriteToFile(repo_file_path, req.content(), start);
  }

  int32_t CreateRepoFile(const common::ReceiveContext& receive_ctx) {
    proto::RepoFile file;
    file.set_file_name(receive_ctx.file_name);
    file.set_file_hash(receive_ctx.file_hash);

    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(receive_ctx.repo_uuid, &repo_meta)) {
      LOG(ERROR) << "Cannot find repo meta: " << receive_ctx.repo_uuid;
      return Err_Repo_meta_not_exists;
    }

    absl::base_internal::SpinLockHolder locker(&lock_);
    auto it = repo_datas_.find(receive_ctx.repo_uuid);
    if (it != repo_datas_.end()) {
      auto dir_it = it->second.mutable_dirs()->find(repo_meta.repo_path());
      if (dir_it == it->second.dirs().end()) {
        proto::RepoDir dir;
        dir.set_path(receive_ctx.dst);
        dir.mutable_files()->emplace(receive_ctx.file_name, file);
      } else {
        dir_it->second.mutable_files()->emplace(receive_ctx.file_name, file);
      }
    } else {
      proto::RepoData repo_data;
      repo_data.set_repo_name(repo_meta.repo_name());
      repo_data.set_repo_path(repo_meta.repo_path());
      repo_data.set_repo_uuid(repo_meta.repo_uuid());
      repo_data.set_repo_location_uuid(repo_meta.repo_location_uuid());
      proto::RepoDir dir;
      dir.set_path(receive_ctx.dst);
      dir.mutable_files()->emplace(receive_ctx.file_name, file);
      repo_data.mutable_dirs()->emplace(receive_ctx.dst, dir);
    }
    return Err_Success;
  }

 private:
  proto::Repos repos_;
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::shared_mutex mutex_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::map<std::string, proto::RepoData> repo_datas_;
  std::thread dump_task_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
