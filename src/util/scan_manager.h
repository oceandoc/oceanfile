/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H

#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/base/internal/spinlock.h"
#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/proto/data.pb.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class ScanManager {
 private:
  friend class folly::Singleton<ScanManager>;
  ScanManager() = default;

 public:
  static std::shared_ptr<ScanManager> Instance();

  bool Init() {
    std::filesystem::path path = std::filesystem::current_path();
    path += (std::filesystem::path::preferred_separator);
    path += "conf";
    path += std::filesystem::path::preferred_separator;
    path += "repos.json";

    std::string content;
    auto ret = Util::LoadSmallFile(path.string(), &content);
    if (ret && !Util::JsonToMessage(content, &repos_)) {
      LOG(ERROR) << "Read repo config error, path: " << path.string()
                 << ", content: " << content;
      return false;
    }
    return true;
  }

  bool Scan(const std::string& path) {
    if (!std::filesystem::exists(path)) {
      LOG(ERROR) << path << " not exists";
      return false;
    }
    if (scanning_.count(path)) {
      LOG(ERROR) << path << " already in scanning...";
      return false;
    }

    auto it = status_.find(path);
    if (it == status_.end()) {
      proto::ScanStatus status;
      status.set_path(path);
      status_.emplace(path, std::move(status));
      return Scan(path, &status_[path]);
    } else {
      return Scan(path, &it->second);
    }
    return false;
  }

  bool Scan(const std::string& path, proto::ScanStatus* scan_status) {
    auto it = scanned_dirs_.find(path);
    if (it != scanned_dirs_.end()) {
      return true;
    }

    try {
      int symlink_num = 0;
      int file_num = 0;
      int dir_num = 0;
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_symlink()) {
          ++symlink_num;
        } else if (entry.is_regular_file()) {
          ++file_num;
        } else if (entry.is_directory()) {
          ++dir_num;
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }

      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        scan_status->set_symlink_num(scan_status->symlink_num() + symlink_num);
        scan_status->set_file_num(scan_status->file_num() + file_num);
        scan_status->set_dir_num(scan_status->file_num() + dir_num);
      }

      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
          scan_status->set_dir_num(scan_status->dir_num() + 1);
          if (!Scan(entry.path().string(), scan_status)) {
            return false;
          }
        }
      }

      scanned_dirs_.insert(path);
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }
    return true;
  }

  bool ParallelScan(const std::string& path, proto::ScanStatus* scan_status) {
    auto it = scanned_dirs_.find(path);
    if (it != scanned_dirs_.end()) {
      return true;
    }

    auto s_path = std::filesystem::path(path);

    try {
      int symlink_num = 0;
      int file_num = 0;
      int dir_num = 0;
      for (const auto& entry : std::filesystem::directory_iterator(s_path)) {
        if (entry.is_symlink()) {
          ++symlink_num;
        } else if (entry.is_regular_file()) {
          ++file_num;
        } else if (entry.is_directory()) {
          ++dir_num;
        } else {
          LOG(ERROR) << "Unknow file type: " << entry.path();
        }
      }

      {
        absl::base_internal::SpinLockHolder locker(&lock_);
        scan_status->set_symlink_num(scan_status->symlink_num() + symlink_num);
        scan_status->set_file_num(scan_status->file_num() + file_num);
        scan_status->set_dir_num(scan_status->file_num() + dir_num);
      }

      if (dir_num <= 0) {
        return true;
      }

      if (dir_num == 1) {
        for (const auto& entry : std::filesystem::directory_iterator(s_path)) {
          if (!entry.is_directory()) {
            continue;
          }
          ParallelScan(entry.path().string(), scan_status);
        }
      }

      for (auto it = std::filesystem::directory_iterator(path);
           it != std::filesystem::directory_iterator();) {
        if (!it->is_directory()) {
          continue;
        }

        auto cur_thread_next_dir = it->path().string();
        ++it;

        std::vector<std::future<bool>> rets;
        while (current_threads <= max_threads &&
               it != std::filesystem::directory_iterator()) {
          std::packaged_task<bool()> task(std::bind(
              static_cast<bool (ScanManager::*)(
                  const std::string&, proto::ScanStatus*)>(&ScanManager::Scan),
              this, it->path().string(), scan_status));
          rets.emplace_back(task.get_future());
          ThreadPool::Instance()->Post(task);
          ++it;
          absl::base_internal::SpinLockHolder locker(&lock_);
          ++current_threads;
        }
        ParallelScan(cur_thread_next_dir, scan_status);
        for (auto& f : rets) {
          if (f.get() == false) {
            return false;
          }
          absl::base_internal::SpinLockHolder locker(&lock_);
          --current_threads;
        }
      }

      scanned_dirs_.insert(path);
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(ERROR) << "Scan " << path << " error: " << e.what();
      return false;
    }
    return true;
  }

  void Print(const std::string& path) {
    auto it = status_.find(path);
    if (it != status_.end()) {
      Util::PrintProtoMessage(it->second);
    } else {
      LOG(ERROR) << path << " not scanned";
    }
  }

 private:
  proto::OceanRepo repos_;
  std::map<std::string, proto::ScanStatus> status_;
  std::unordered_set<std::string> scanned_dirs_;
  std::set<std::string> scanning_;
  mutable absl::base_internal::SpinLock lock_;
  int current_threads = 0;
  int max_threads = 3;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
