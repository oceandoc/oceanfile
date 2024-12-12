/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
#define BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H

#include "src/impl/repo_manager.h"
#include "src/impl/server_manager.h"
#include "src/impl/session_manager.h"
#include "src/impl/user_manager.h"
#include "src/proto/service.pb.h"
#include "src/server/handler_proxy/file_handler.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class HandlerProxy {
 public:
  static void FileOpHandle(const proto::FileReq& req, proto::FileRes* res) {
    FileHandler::FileOpHandle(req, res);
  }

  static void UserOpHandle(const proto::UserReq& req, proto::UserRes* res) {
    std::string json;
    util::Util::MessageToJson(req, &json);
    LOG(INFO) << json;
    std::string session_user;
    if (req.token().empty() && req.op() != proto::UserOp::UserCreate &&
        req.op() != proto::UserOp::UserLogin) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
    }
    if (!req.token().empty() &&
        !impl::SessionManager::Instance()->ValidateSession(req.token(),
                                                           &session_user)) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      return;
    }
    if (!session_user.empty() && session_user != req.user()) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      return;
    }

    int32_t ret = Err_Success;
    switch (req.op()) {
      case proto::UserOp::UserCreate:
        ret = impl::UserManager::Instance()->UserRegister(
            req.user(), req.password(), res->mutable_token());
        break;
      case proto::UserOp::UserDelete:
        ret = impl::UserManager::Instance()->UserDelete(
            session_user, req.to_delete_user(), req.token());
        break;
      case proto::UserOp::UserLogin:
        ret = impl::UserManager::Instance()->UserLogin(
            req.user(), req.password(), res->mutable_token());
        break;
      case proto::UserOp::UserChangePassword:
        ret = impl::UserManager::Instance()->ChangePassword(
            req.user(), req.old_password(), req.password(),
            res->mutable_token());
        break;
      case proto::UserOp::UserLogout:
        ret = impl::UserManager::Instance()->UserLogout(req.token());
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
  }

  static void RepoOpHandle(const proto::RepoReq& req, proto::RepoRes* res) {
    if (req.token().empty()) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      return;
    }

    std::string session_user;
    if (!impl::SessionManager::Instance()->ValidateSession(req.token(),
                                                           &session_user)) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
      return;
    }
    if (!session_user.empty() && session_user != req.user()) {
      res->set_err_code(proto::ErrCode(Err_User_session_error));
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
      default:
        ret = Err_Unsupported_op;
        LOG(ERROR) << "Unsupported operation";
    }
    if (ret) {
      res->set_err_code(proto::ErrCode(ret));
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }
  }

  static void ServerOpHandle(const proto::ServerReq& req,
                             proto::ServerRes* res) {
    int32_t ret = Err_Success;
    switch (req.op()) {
      case proto::ServerOp::ServerHandShake:
        ret = impl::ServerManager::Instance()->HandShake(req, res);
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
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
