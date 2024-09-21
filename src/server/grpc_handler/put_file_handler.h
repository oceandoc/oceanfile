/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/util/repo_manager.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class PutFileHandler : public async_grpc::RpcHandler<PutFileMethod> {
 public:
  void OnRequest(const proto::FileReq& req) override {
    auto res = std::make_unique<proto::FileRes>();

    if (req.partition_num() == 0) {
    }

    auto ret = util::RepoManager::Instance()->WriteToFile(
        req.repo_uuid(), req.sha256(), req.content(), true);

    Send(std::move(res));
  }

  void OnReadsDone() override { Finish(grpc::Status::OK); }

 private:
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_PUT_FILE_HANDLER_H
