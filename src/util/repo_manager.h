/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

#include "folly/Singleton.h"
#include "glog/logging.h"
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
    std::string path = "./data/repos.json";
    std::string content;
    auto ret = Util::LoadSmallFile(path, &content);
    absl::base_internal::SpinLockHolder locker(&lock_);
    if (ret && !Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repo config error, path: " << path
                 << ", content: " << content;
      return false;
    }
    return true;
  }

  bool CreateRepo(const std::string& path) {
    std::string uuid = Util::UUID();
    const std::string& repo_file_path =
        path + "/" + ScanManager::mark_dir_name + "/" + uuid + ".repo";
    proto::Repo repo;
    repo.set_create_time(Util::CurrentTimeMillis());
    repo.set_uuid(uuid);

    {
      absl::base_internal::SpinLockHolder locker(&lock_);
      repos_.mutable_repos()->insert({uuid, repo});
    }

    std::string content, compressed_content;
    repo.SerializeToString(&content);
    Util::LZMACompress(content, &compressed_content);
    return Util::WriteToFile(repo_file_path, compressed_content, false);
  }

  std::string RepoPath(const std::string& uuid) {
    auto it = repos_.repos().find(uuid);
    if (it != repos_.repos().end()) {
      return it->second.path();
    }
    return "";
  }

  bool WriteToFile(const std::string& repo_uuid, const std::string& sha256,
                   const std::string& content, const int64_t start,
                   const int64_t end) {
    const auto& rep_path = RepoPath(repo_uuid);
    const auto& repo_file_path = Util::RepoFilePath(rep_path, sha256);
    return Util::WriteToFile(repo_file_path, content, append);
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
    Util::Mkdir(canonical_dst);

    proto::ScanStatus scan_status;
    std::unordered_set<std::string> copy_failed_files;
    std::unordered_set<std::string> scanned_dirs;
    scan_status.mutable_ignored_dirs()->insert(
        {ScanManager::mark_dir_name, true});
    auto ret = ScanManager::Instance()->ParallelScan(
        canonical_src, &scan_status, &scanned_dirs, disable_scan_cache);
    if (!ret) {
      LOG(ERROR) << "Scan " << src << " error";
      return false;
    }

    if (scan_status.complete_time() == 0) {
      LOG(ERROR) << "Scan error: " << src;
      return false;
    }

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
        LOG(INFO) << "Exists error";
      }
    }
    ScanManager::Instance()->Print(scan_status);
    for (const auto& file : copy_failed_files) {
      LOG(ERROR) << file << " sync failed";
    }
    return success;
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

      if (dir.filename() == ScanManager::mark_dir_name) {
        continue;
      }

      std::string relative_path;
      Util::Relative(dir.string(), src, &relative_path);
      auto dst_path = std::filesystem::path(dst + "/" + relative_path);

      if (std::filesystem::is_symlink(dir)) {
        LOG(ERROR) << dir.string() << " symlink";
      }

      if (std::filesystem::exists(dst_path)) {
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
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
