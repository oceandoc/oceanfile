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
  void OnRequest(const proto::StatusReq& req) override {
    LOG(INFO) << req.request_id();
    auto res = std::make_unique<proto::StatusRes>();
    res->set_status(
        static_cast<oceandoc::server::ServerContext*>(execution_context_)
            ->ToString());
    Send(std::move(res));
  }
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_STATUS_HANDLER_H
