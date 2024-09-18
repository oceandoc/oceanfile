/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H

#include <memory>
#include <string>

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
    std::string path = "./data/repos/repos.json";
    std::string content;
    auto ret = Util::LoadSmallFile(path, &content);
    if (ret && !Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repo config error, path: " << path
                 << ", content: " << content;
      return false;
    }
    return true;
  }

  bool SyncLocal(std::string src, std::string dst,
                 bool disable_scan_cache = false) {
    auto status =
        ScanManager::Instance()->ParallelScan(src, disable_scan_cache);
    if (status.complete_time() == 0) {
      LOG(ERROR) << "Scan error: " << src;
      return false;
    }

    bool success = true;
    std::vector<std::future<bool>> rets;
    std::packaged_task<bool()> task(std::bind(
        static_cast<bool (RepoManager::*)(
            const int no, const proto::ScanStatus&)>(&RepoManager::SyncWorker),
        this, 1, status));
    rets.emplace_back(task.get_future());
    ThreadPool::Instance()->Post(task);
    for (auto& f : rets) {
      if (f.get() == false) {
        success = false;
      }
    }

    return success;
  }

  bool SyncWorker(const int no, const proto::ScanStatus& status) {
    bool success = true;
    for (int i = no; i < status.scanned_files().size(); i += max_threads) {
      auto ret =
          Util::CopyFile(status.scanned_files(i).path(), "test",
                         std::filesystem::copy_options::overwrite_existing);
      if (!ret) {
        copy_failed_files_.insert(status.scanned_files(i).path());
        success = false;
      }
    }
    return success;
  }

 private:
  proto::OceanRepo repos_;
  int current_threads = 0;
  const int max_threads = 5;
  std::set<std::string> copy_failed_files_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
