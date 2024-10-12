/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/server/handler_proxy/handler_proxy.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class FileHandler : public async_grpc::RpcHandler<FileOpMethod> {
 public:
  void OnRequest(const proto::FileReq& req) override {
    auto res = std::make_unique<proto::FileRes>();
    handler_proxy::HandlerProxy::FileOpHandle(req, res.get());
    Send(std::move(res));
  }

  void OnReadsDone() override { Finish(grpc::Status::OK); }

 private:
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H
