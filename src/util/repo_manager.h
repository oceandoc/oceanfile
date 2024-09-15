/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H

#include <memory>
#include <string>
#include <string_view>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/proto/data.pb.h"
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

 private:
  proto::OceanRepo repos_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_REPO_MANAGER_H
