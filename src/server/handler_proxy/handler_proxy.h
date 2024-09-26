/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
#define BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H

#include "src/proto/service.pb.h"
#include "src/util/repo_manager.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class HandlerProxy {
 public:
  static void FileOpFailResponse(proto::FileRes* res) {
    res->set_err_code(proto::ErrCode::FAIL);
  }

  static void FileOpHandle(const proto::FileReq& req, proto::FileRes* res) {
    bool ret = true;
    switch (req.op()) {
      case proto::FileOp::FilePut:
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
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
