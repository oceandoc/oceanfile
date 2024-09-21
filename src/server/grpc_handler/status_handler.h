/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_STATUS_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_STATUS_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/server/server_context.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class StatusHandler : public async_grpc::RpcHandler<StatusMethod> {
 public:
  void OnRequest(const proto::StatusReq& request) override {
    LOG(INFO) << request.request_id() << ", "
              << request.context().private_ipv4();
    auto response = std::make_unique<proto::StatusRes>();
    response->set_msg(
        static_cast<oceandoc::server::ServerContext*>(execution_context_)
            ->ToString());
    Send(std::move(response));
  }
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_STATUS_HANDLER_H
