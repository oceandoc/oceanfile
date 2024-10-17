/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_REPO_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_REPO_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/impl/repo_manager.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class RepoHandler : public async_grpc::RpcHandler<RepoOpMethod> {
 public:
  void OnRequest(const proto::RepoReq& req) override {
    auto res = std::make_unique<proto::RepoRes>();
    std::string uuid;
    bool ret = true;
    switch (req.op()) {
      case proto::RepoOp::RepoCreate:
        ret = impl::RepoManager::Instance()->CreateRepo(req.path(), &uuid);
        break;
      default:
        LOG(ERROR) << "Unsupported operation";
    }

    if (!ret) {
      res->set_err_code(proto::ErrCode::Fail);
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }
    res->set_repo_uuid(uuid);
    Send(std::move(res));
  }

  void OnReadsDone() override { Finish(grpc::Status::OK); }

 private:
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_REPO_HANDLER_H
