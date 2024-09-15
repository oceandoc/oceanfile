/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CONTEXT_SERVER_CONTEXT_H
#define BAZEL_TEMPLATE_CONTEXT_SERVER_CONTEXT_H

#include <atomic>
#include <future>

#include "fmt/core.h"
#include "glog/logging.h"
#include "src/async_grpc/execution_context.h"
#include "src/server/version_info.h"
#include "src/util/config_manager.h"

namespace oceandoc {
namespace server {

using EchoResponder = std::function<bool()>;

class ServerContext : public async_grpc::ExecutionContext {
 public:
  ServerContext() : is_inited_(false), git_commit_(GIT_VERSION) {}

  void MarkedServerInitedDone() {
    is_inited_.store(true);
    LOG(INFO) << "grpc server started on: "
              << util::ConfigManager::Instance()->ServerAddr() << ", port: "
              << util::ConfigManager::Instance()->GrpcServerPort();
  }

  bool IsInitYet() { return is_inited_.load(); }

  std::string ToString() {
    std::string info;
    info.reserve(1024);
    info.append(fmt::format("uptime: {}\n", util::Util::DetailTimeStr()));
    info.append(fmt::format("git commit: {}\n", git_commit_));
    return info;
  }

 public:
  std::promise<EchoResponder> echo_responder;

 private:
  std::atomic_bool is_inited_;
  std::string git_commit_;
};

}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CONTEXT_SERVER_CONTEXT_H
