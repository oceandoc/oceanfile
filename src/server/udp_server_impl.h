/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H
#define BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H

#include <memory>
#include <string>

#include "src/server/server_context.h"
#include "src/util/config_manager.h"

namespace oceandoc {
namespace server {

class UdpServer final {
 public:
  UdpServer(std::shared_ptr<ServerContext> server_context)
      : server_context_(server_context) {
    std::string addr = util::ConfigManager::Instance()->ServerAddr();
    int32_t port = util::ConfigManager::Instance()->UdpServerPort();
  }

 public:
  void Start() { server_context_->MarkedUdpServerInitedDone(); }

  void Shutdown() {}

 private:
  std::shared_ptr<ServerContext> server_context_;
};

}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H
