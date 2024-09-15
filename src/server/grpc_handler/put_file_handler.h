/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class PutFileHandler : public async_grpc::RpcHandler<PutFileMethod> {
 public:
  void OnRequest(const proto::FileReq& request) override {
    // Respond twice to demonstrate bidirectional streaming.
    auto response = std::make_unique<proto::FileRes>();
    Send(std::move(response));
  }

  void OnReadsDone() override { Finish(grpc::Status::OK); }

 private:
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H
