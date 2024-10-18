/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/server/server_context.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class ServerHandler : public async_grpc::RpcHandler<ServerMethod> {
 public:
  void OnRequest(const proto::ServerReq& req) override {
    LOG(INFO) << "qid: " << req.request_id();
    auto res = std::make_unique<proto::ServerRes>();
    res->set_status(
        static_cast<oceandoc::server::ServerContext*>(execution_context_)
            ->ToString());
    Send(std::move(res));
  }

  void OnReadsDone() override {
    LOG(INFO) << "OnReadsDone";
    Finish(grpc::Status::OK);
  }
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_SERVER_HANDLER_H
