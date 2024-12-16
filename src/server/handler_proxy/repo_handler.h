/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_REPO_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_REPO_HANDLER_H

#include "src/impl/repo_manager.h"
#include "src/impl/session_manager.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class RepoHandler {
 public:
  static void RepoOpHandle(const proto::RepoReq& req, proto::RepoRes* res) {
    LOG(INFO) << "repo req: " << util::Util::MessageToJson(req);
    if (req.token().empty()) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      LOG(ERROR) << "User token empty: " << req.user();
      return;
    }

    std::string session_user;
    if (!impl::SessionManager::Instance()->ValidateSession(req.token(),
                                                           &session_user)) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      LOG(ERROR) << "User token wrong: " << req.user();
      return;
    }
    if (!session_user.empty() && session_user != req.user()) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      LOG(ERROR) << "User name mismatch: " << req.user();
      return;
    }

    int32_t ret = Err_Success;
    switch (req.op()) {
      case proto::RepoOp::RepoListUserRepo:
        ret = impl::RepoManager::Instance()->ListUserRepo(req, res);
        break;
      case proto::RepoOp::RepoListServerDir:
        ret = impl::RepoManager::Instance()->ListServerDir(req, res);
        break;
      case proto::RepoOp::RepoCreateServerDir:
        ret = impl::RepoManager::Instance()->CreateServerDir(req, res);
        break;
      case proto::RepoOp::RepoCreateRepo:
        ret = impl::RepoManager::Instance()->CreateRepo(req, res);
        break;
      case proto::RepoOp::RepoDeleteRepo:
        ret = impl::RepoManager::Instance()->DeleteRepo(req, res);
        break;
      case proto::RepoOp::RepoListRepoDir:
        ret = impl::RepoManager::Instance()->ListRepoDir(req, res);
        break;
      case proto::RepoOp::RepoListRepoMediaFiles:
        ret = impl::RepoManager::Instance()->ListRepoDir(req, res);
        break;
      default:
        ret = Err_Unsupported_op;
        LOG(ERROR) << "Unsupported operation";
    }
    if (ret) {
      res->set_err_code(proto::ErrCode(ret));
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }
    LOG(INFO) << "repo res: " << util::Util::MessageToJson(*res);
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_REPO_HANDLER_H
