/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_IMPL_SERVER_MANAGER_H
#define BAZEL_TEMPLATE_IMPL_SERVER_MANAGER_H

#include <cstdlib>
#include <memory>

#include "folly/Singleton.h"
#include "src/common/error.h"
#include "src/proto/service.pb.h"
#include "src/util/config_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace impl {

class ServerManager {
 private:
  friend class folly::Singleton<ServerManager>;
  ServerManager() = default;

 public:
  static std::shared_ptr<ServerManager> Instance();

  bool Init() { return true; }

  uint32_t HandShake(const proto::ServerReq& /* req */, proto::ServerRes* res) {
    res->set_handshake_msg(
        "7a3be8186493f1bc834e3a6b84fcb2f9dc6d042e93d285ec23fa56836889dfa9");
    res->set_server_uuid(util::ConfigManager::Instance()->ServerUUID());
    return Err_Success;
  }

 private:
  std::atomic<uint32_t> scanning_ = 0;
  std::atomic<bool> stop_ = false;
};

}  // namespace impl
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_IMPL_SERVER_MANAGER_H
