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
#include "src/impl/file_process_manager.h"
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
    LOG(INFO) << "Restored success" << repo->repo_uuid();

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      if (repos_.repos().find(repo->repo_uuid()) != repos_.repos().end()) {
        LOG(INFO) << "Repo already exists: " << repo->repo_uuid();
        return Err_Success;
      }
      (*repos_.mutable_repos())[repo->repo_uuid()] = *repo;
    }
    return FlushRepoMeta();
  }

  int32_t CreateRepo(const proto::RepoReq& req, proto::RepoRes* res) {
    proto::RepoMeta repo;
    if (ExistsRepo(req.path(), repo.mutable_repo_uuid())) {
      if (RestoreRepo(req.path(), &repo) == Err_Success) {
        *res->mutable_repo() = repo;
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

  int32_t ListRepoMediaFiles(const proto::RepoReq& /*req*/,
                             proto::RepoRes* res) {
    sqlite3_stmt* stmt = nullptr;
    // std::string query =
    //"SELECT hash, local_id, device_id, create_time, update_time, "
    //"duration, type, width, height, file_name, favorite, owner, "
    //"live_photo_video_hash, deleted, thumb_hash "
    //"FROM files WHERE type IN (1, 2) AND deleted = 0 "
    //"ORDER BY create_time DESC;";
    std::string query =
        "SELECT hash, local_id, device_id, create_time, update_time, "
        "duration, type, width, height, file_name, favorite, owner, "
        "live_photo_video_hash, deleted, thumb_hash "
        "FROM files;";

    auto ret = util::SqliteManager::Instance()->PrepareStatement(query, &stmt);
    if (ret != Err_Success) {
      return Err_Fail;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      auto* file_info = res->add_file_infos();

      // Get values from result row
      const char* hash = (const char*)sqlite3_column_text(stmt, 0);
      const char* local_id = (const char*)sqlite3_column_text(stmt, 1);
      const char* device_id = (const char*)sqlite3_column_text(stmt, 2);
      int64_t create_time = sqlite3_column_int64(stmt, 3);
      int64_t update_time = sqlite3_column_int64(stmt, 4);
      int64_t duration = sqlite3_column_int64(stmt, 5);
      int32_t type = sqlite3_column_int(stmt, 6);
      int32_t width = sqlite3_column_int(stmt, 7);
      int32_t height = sqlite3_column_int(stmt, 8);
      const char* file_name = (const char*)sqlite3_column_text(stmt, 9);
      int32_t favorite = sqlite3_column_int(stmt, 10);
      const char* owner = (const char*)sqlite3_column_text(stmt, 11);
      const char* live_photo_hash = (const char*)sqlite3_column_text(stmt, 12);
      const char* thumb_hash = (const char*)sqlite3_column_text(stmt, 14);

      // Set values in protobuf message
      if (hash) file_info->set_hash(hash);
      if (local_id) file_info->set_local_id(local_id);
      if (device_id) file_info->set_device_id(device_id);
      file_info->set_create_time(create_time);
      file_info->set_update_time(update_time);
      file_info->set_duration(duration);
      file_info->set_type(type);
      file_info->set_width(width);
      file_info->set_height(height);
      if (file_name) file_info->set_file_name(file_name);
      file_info->set_favorite(favorite);
      if (owner) file_info->set_owner(owner);
      if (live_photo_hash)
        file_info->set_live_photo_video_hash(live_photo_hash);
      if (thumb_hash) file_info->set_thumb_hash(thumb_hash);
    }

    sqlite3_finalize(stmt);
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

  int32_t ReadFile(const proto::FileReq& req, std::string* content) {
    if (req.repo_uuid().empty()) {
      LOG(ERROR) << "Repo uuid empty";
      return Err_Repo_uuid_error;
    }
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
    auto ret = util::Util::WriteToFile(repo_file_path, req.content(), start);
    if (ret) {
      LOG(ERROR) << "Store part error, "
                 << "file: " << req.file_hash()
                 << ", part: " << req.partition_num();
    } else {
      LOG(INFO) << "Store part success, "
                << "file: " << req.file_hash()
                << ", part: " << req.partition_num()
                << ", size: " << req.file_size();
      impl::FileProcessManager::Instance()->Put(req);
    }
    return ret;
  }
  int32_t WriteToFile(const proto::FileReq& req) {
    if (req.repo_uuid().empty()) {
      LOG(ERROR) << "Repo uuid empty";
      return Err_Repo_uuid_error;
    }
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
    auto ret = util::Util::WriteToFile(repo_file_path, req.content(), start);
    if (ret) {
      LOG(ERROR) << "Store part error, "
                 << "file: " << req.file_hash()
                 << ", part: " << req.partition_num();
    } else {
      LOG(INFO) << "Store part success, "
                << "file: " << req.file_hash()
                << ", part: " << req.partition_num()
                << ", size: " << req.file_size();
      impl::FileProcessManager::Instance()->Put(req);
    }
    return ret;
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
  std::string repos_config_path_;
  std::string tmp_repos_config_path_;
  proto::Repos repos_;
  std::map<std::string, proto::RepoData> repo_datas_;
  mutable absl::base_internal::SpinLock lock_;
  std::atomic<bool> stop_ = false;
  std::shared_mutex mutex_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::thread dump_task_;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_REPO_MANAGER_H
