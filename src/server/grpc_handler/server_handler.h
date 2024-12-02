/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/server/handler_proxy/handler_proxy.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class ServerHandler : public async_grpc::RpcHandler<ServerMethod> {
 public:
  void OnRequest(const proto::ServerReq& req) override {
    LOG(INFO) << "request_id: " << req.request_id();
    auto res = std::make_unique<proto::ServerRes>();
    handler_proxy::HandlerProxy::ServerOpHandle(req, res.get());
    Send(std::move(res));
  }

  void OnReadsDone() override { Finish(grpc::Status::OK); }
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H
