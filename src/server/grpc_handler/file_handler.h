/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H

#include "src/async_grpc/rpc_handler.h"
#include "src/proto/service.pb.h"
#include "src/server/grpc_handler/meta.h"
#include "src/util/repo_manager.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

class FileHandler : public async_grpc::RpcHandler<FileOpMethod> {
 public:
  void OnRequest(const proto::FileReq& req) override {
    auto res = std::make_unique<proto::FileRes>();
    bool ret = true;
    switch (req.op()) {
      case proto::Op::File_Put:
        if (req.repo_uuid().empty()) {
          ret = false;
          LOG(ERROR) << "Repo uuid empty";
        } else {
          ret = util::RepoManager::Instance()->WriteToFile(
              req.repo_uuid(), req.sha256(), req.content(), req.size(),
              req.partition_num());
        }
        break;
      default:
        LOG(ERROR) << "Unsupported operation";
    }

    if (!ret) {
      LOG(INFO) << "Store file error: " << req.sha256()
                << ", part: " << req.partition_num();
      res->set_err_code(proto::ErrCode::FAIL);
    } else {
      res->set_err_code(proto::ErrCode::SUCCESS);
    }
    res->set_path(req.path());
    res->set_sha256(req.sha256());
    res->set_partition_num(req.partition_num());
    Send(std::move(res));
  }

  void OnReadsDone() override {
    LOG(INFO) << "OnReadsDone";
    Finish(grpc::Status::OK);
  }

 private:
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_FILE_HANDLER_H
