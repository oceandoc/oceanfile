/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
#define BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H

#include "src/proto/service.pb.h"
#include "src/util/repo_manager.h"
#include "src/util/sync_manager.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class HandlerProxy {
 public:
  static void FileOpFailResponse(proto::FileRes* res) {
    res->set_err_code(proto::ErrCode::Fail);
  }

  static bool SaveFile(const proto::FileReq& req, proto::FileRes* res) {
    bool ret = true;
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      if (req.repo_uuid().empty()) {
        ret = false;
        LOG(ERROR) << "Repo uuid empty";
      } else {
        ret = util::RepoManager::Instance()->WriteToFile(req);
      }
    } else if (req.repo_type() == proto::RepoType::RT_Remote) {
      LOG(INFO) << "repo";
      ret = util::SyncManager::Instance()->WriteToFile(req);
    }
    return ret;
  }

  static void FileOpHandle(const proto::FileReq& req, proto::FileRes* res) {
    bool ret = true;
    switch (req.op()) {
      case proto::FileOp::FilePut:
        ret = SaveFile(req, res);
        break;
      default:
        LOG(ERROR) << "Unsupported operation";
    }

    if (!ret) {
      LOG(ERROR) << "Store file error: " << req.hash()
                 << ", part: " << req.partition_num();
      res->set_err_code(proto::ErrCode::Fail);
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }
    res->set_path(req.path());
    res->set_hash(req.hash());
    res->set_partition_num(req.partition_num());
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
