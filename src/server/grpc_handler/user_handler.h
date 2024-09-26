/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_USER_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_USER_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/server/server_context.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class UserHandler : public async_grpc::RpcHandler<UserMethod> {
 public:
  void OnRequest(const proto::UserReq& req) override {
    LOG(INFO) << "qid: " << req.request_id();
    auto res = std::make_unique<proto::UserRes>();
    res->set_repo_uuid(
        static_cast<oceandoc::server::ServerContext*>(execution_context_)
            ->ToString());
    Send(std::move(res));
  }
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_USER_HANDLER_H
