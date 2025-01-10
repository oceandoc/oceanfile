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
#include <utility>
#include <vector>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/common/error.h"
#include "src/impl/file_process_manager.h"
#include "src/proto/data.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/sqlite_row.h"
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
    if (flush_task_.joinable()) {
      flush_task_.join();
    }
  }

  bool Init(const std::string& home_dir) {
    repos_config_path_ = home_dir + "/data/repos.json";
    tmp_repos_config_path_ = home_dir + "/data/repos.tmp.json";
    if (util::Util::Exists(repos_config_path_)) {
      std::string content;
      auto ret = util::Util::LoadSmallFile(repos_config_path_, &content);

      absl::base_internal::SpinLockHolder locker(&lock_);
      if (ret && !util::Util::JsonToMessage(content, &repos_)) {
        LOG(ERROR) << "Read repos config error, content: " << content;
        return false;
      }
    }
    flush_task_ = std::thread(&RepoManager::FlushRepoMetaTask, this);
    LOG(INFO) << "Loaded repo num: " << repos_.repos().size();
    return true;
  }

  void Stop() {
    stop_.store(true);
    cv_.notify_all();
  }

  bool FlushRepoMeta() {
    bool copy_tmp = false;
    if (util::Util::Exists(repos_config_path_)) {
      auto ret =
          util::Util::CopyFile(repos_config_path_, tmp_repos_config_path_);
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

    if (util::Util::WriteToFile(repos_config_path_, content, false) ==
            Err_Success ||
        util::Util::WriteToFile(repos_config_path_, content, false) ==
            Err_Success ||
        util::Util::WriteToFile(repos_config_path_, content, false) ==
            Err_Success) {
      LOG(INFO) << "Flush repos config success, num: " << repos_.repos_size();
      return true;
    }
    if (util::Util::Exists(tmp_repos_config_path_) && copy_tmp) {
      util::Util::CopyFile(tmp_repos_config_path_, repos_config_path_);
      LOG(ERROR) << "Disaster: flush repos config error";
    }
    return false;
  }

  bool ShouldFlushRepoMeta() {
    if (repo_meta_change_num.load()) {
      return true;
    }
    return false;
  }

  void FlushRepoMetaTask() {
    LOG(INFO) << "FlushRepoMetaTask running";
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
      if (ShouldFlushRepoMeta()) {
        FlushRepoMeta();
        repo_meta_change_num.store(0);
      }
    }
    LOG(INFO) << "FlushRepoMetaTask exists";
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
    std::string path = req.dir();
    if (path.empty()) {
      path = "/";
    }

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
        res->mutable_files()->Add(std::forward<proto::File>(file));
        // res->mutable_files()->Add(std::move(file));
      }
    } catch (const std::filesystem::filesystem_error& e) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t CreateServerDir(const proto::RepoReq& req, proto::RepoRes* /*res*/) {
    std::string path = req.dir();
    if (path.empty()) {
      path = "/";
    }

    if (util::Util::Mkdir(path)) {
      return Err_Success;
    }
    return Err_Fail;
  }

  int32_t CreateRepo(const proto::RepoReq& req, proto::RepoRes* res) {
    proto::RepoMeta repo;
    if (ExistsRepo(req.dir(), repo.mutable_repo_uuid())) {
      if (RestoreRepo(req.dir(), &repo) == Err_Success) {
        res->mutable_repos()->insert({repo.repo_uuid(), repo});
        return Err_Success;
      }
      return Err_Fail;
    }

    repo.set_repo_uuid(util::Util::UUID());
    std::string repo_meta_file_path =
        req.dir() + "/" + common::CONFIG_DIR + "/" + repo.repo_uuid() + ".repo";

    repo.set_create_time(
        util::Util::ToTimeStrUTC(util::Util::CurrentTimeMillis()));
    repo.set_update_time(repo.create_time());
    repo.set_repo_location(req.dir());
    repo.set_repo_name(req.repo_name());
    repo.set_repo_location_uuid(util::Util::UUID());
    repo.set_owner(req.user());

    std::string content;
    if (!util::Util::MessageToJson(repo, &content)) {
      LOG(ERROR) << "Convert repo meto to json error";
      return Err_Fail;
    }

    auto ret = util::Util::WriteToFile(repo_meta_file_path, content, false);
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
      return Err_Fail;
    }

    res->mutable_repos()->insert({repo.repo_uuid(), repo});
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
    return Err_Fail;
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

  int32_t ListRepoDir(const proto::RepoReq& req, proto::RepoRes* res) {
    if (req.repo_uuid().empty()) {
      return Err_Fail;
    }

    if (GetRepoDir(req, res)) {
      return Err_Fail;
    }
    return Err_Success;
  }

  int32_t ListRepoMediaFiles(const proto::RepoReq& /*req*/,
                             proto::RepoRes* /*res*/) {
    // std::string query =
    //"SELECT hash, local_id, device_id, create_time, update_time, "
    //"duration, type, width, height, file_name, favorite, owner, "
    //"live_photo_video_hash, deleted, thumb_hash "
    //"FROM files WHERE type IN (1, 2) AND deleted = 0 "
    //"ORDER BY create_time DESC;";
    std::string sql =
        "SELECT hash, local_id, device_id, create_time, update_time, "
        "duration, type, width, height, file_name, favorite, owner, "
        "live_photo_video_hash, deleted, thumb_hash "
        "FROM files;";

    std::string err_msg;
    std::function<bool(sqlite3_stmt * stmt)> bind_callback =
        [](sqlite3_stmt* /*stmt*/) -> bool { return true; };
    std::vector<util::FilesRow> rows;
    auto ret = util::SqliteManager::Instance()->Select<util::FilesRow>(
        sql, &err_msg, bind_callback, &rows);
    if (ret) {
      return Err_Fail;
    }

    return Err_Success;
  }

  int32_t GetRepoDir(const proto::RepoReq& /*req*/, proto::RepoRes* /*res*/) {
    absl::base_internal::SpinLockHolder locker(&lock_);
    return Err_File_not_exists;
  }

  int32_t RepoFileExists(const proto::RepoReq& /*req*/,
                         proto::RepoRes* /*res*/) {
    return Err_File_not_exists;
  }

  int32_t ReadFile(const proto::FileReq& req, std::string* content) {
    if (req.repo_uuid().empty()) {
      LOG(ERROR) << "Repo uuid empty";
      return Err_Fail;
    }
    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(req.repo_uuid(), &repo_meta)) {
      LOG(ERROR) << "Invalid repo path";
      return Err_Repo_not_exists;
    }
    const auto& repo_location = repo_meta.repo_location();
    const auto& repo_file_path =
        util::Util::RepoFilePath(repo_location, req.file().file_hash());
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return Err_Fail;
    }
    LOG(INFO) << "Now get file: " << repo_file_path;
    if (util::Util::LoadSmallFile(repo_file_path, content)) {
      return Err_Success;
    }

    return Err_Fail;
  }

  int32_t WriteToFile(const proto::FileReq& req) {
    if (req.repo_uuid().empty()) {
      LOG(ERROR) << "Repo uuid empty";
      return Err_Fail;
    }
    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(req.repo_uuid(), &repo_meta)) {
      LOG(ERROR) << "Invalid repo path";
      return Err_Repo_not_exists;
    }
    const auto& repo_location = repo_meta.repo_location();
    const auto& repo_file_path =
        util::Util::RepoFilePath(repo_location, req.file().file_hash());
    if (repo_file_path.empty()) {
      LOG(ERROR) << "Invalid repo file path";
      return Err_Fail;
    }

    util::Util::MkParentDir(repo_file_path);

    auto err_code = Err_Success;
    err_code =
        util::Util::CreateFileWithSize(repo_file_path, req.file().file_size());
    if (err_code != Err_Success) {
      LOG(ERROR) << "Create file error: " << repo_file_path;
      return err_code;
    }

    int64_t start = 0, end = 0;
    util::Util::CalcPartitionStart(req.file().file_size(), req.cur_part(),
                                   req.size_per_part(), &start, &end);
    if (end - start + 1 != static_cast<int64_t>(req.content().size())) {
      LOG(ERROR) << "Calc size error, partition_num: " << req.cur_part()
                 << ", start: " << start << ", end: " << end
                 << ", content size: " << req.content().size();
      return Err_Fail;
    }

    static thread_local std::shared_mutex mu;
    std::unique_lock<std::shared_mutex> locker(mu);
    auto ret = util::Util::WriteToFile(repo_file_path, req.content(), start);
    if (ret) {
      LOG(ERROR) << "Store part error, "
                 << "file: " << req.file().file_hash()
                 << ", part: " << req.cur_part();
    } else {
      LOG(INFO) << "Store part success, "
                << "file: " << req.file().file_hash()
                << ", part: " << req.cur_part()
                << ", size: " << req.file().file_size();
      impl::FileProcessManager::Instance()->Put(req);
    }
    return ret;
  }

  int32_t CreateRepoFile(const common::ReceiveContext& receive_ctx) {
    proto::RepoMeta repo_meta;
    if (!RepoMetaByUUID(receive_ctx.repo_uuid, &repo_meta)) {
      LOG(ERROR) << "Cannot find repo meta: " << receive_ctx.repo_uuid;
      return Err_Fail;
    }

    return Err_Success;
  }

 private:
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
    std::string repo_meta_file_path =
        path + "/" + common::CONFIG_DIR + "/" + repo->repo_uuid() + ".repo";
    LOG(INFO) << "Now restore repo from path: " << repo_meta_file_path;

    if (!util::Util::LoadSmallFile(repo_meta_file_path, &content)) {
      return false;
    }

    if (!util::Util::JsonToMessage(content, repo)) {
      LOG(ERROR) << "Parse repo meta error, content: " << content;
      return false;
    }

    if (path != repo->repo_location()) {
      LOG(WARNING) << "Repo moved, origin path: " << repo->repo_location();
      repo->set_repo_location(path);
    }

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (repos_.repos().find(repo->repo_uuid()) != repos_.repos().end()) {
        LOG(INFO) << "Repo already exists: " << repo->repo_uuid();
        return true;
      }
      (*repos_.mutable_repos())[repo->repo_uuid()] = *repo;
    }

    if (FlushRepoMeta()) {
      LOG(INFO) << "Restored success" << repo->repo_uuid();
      return true;
    }
    return false;
  }

 private:
  std::atomic<bool> stop_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
  std::thread flush_task_;
  std::atomic<uint64_t> repo_meta_change_num = 0;

  std::string repos_config_path_;
  std::string tmp_repos_config_path_;
  proto::Repos repos_;

  mutable absl::base_internal::SpinLock lock_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
