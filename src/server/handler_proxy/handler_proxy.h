/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
#define BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H

#include <filesystem>

#include "src/impl/receive_queue_manager.h"
#include "src/impl/repo_manager.h"
#include "src/impl/server_manager.h"
#include "src/impl/session_manager.h"
#include "src/impl/sync_manager.h"
#include "src/impl/user_manager.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class HandlerProxy {
 public:
  static void FileOpFailResponse(proto::FileRes* res) {
    res->set_err_code(proto::ErrCode::Fail);
  }

  static int32_t Save(const proto::FileReq& req, proto::FileRes* res) {
    int32_t ret = Err_Success;
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      if (req.repo_uuid().empty()) {
        ret = Err_Repo_uuid_error;
        LOG(ERROR) << "Repo uuid empty";
      } else {
        ret = impl::RepoManager::Instance()->WriteToFile(req);
        if (ret) {
          LOG(ERROR) << "Store part error, "
                     << "path: " << req.dst()
                     << ", part: " << req.partition_num();
        } else {
          impl::ReceiveQueueManager::Instance()->Put(req);
        }
      }
    } else if (req.repo_type() == proto::RepoType::RT_Remote) {
      ret = impl::SyncManager::Instance()->WriteToFile(req);
      if (ret) {
        LOG(ERROR) << "Store part error, "
                   << "path: " << req.dst()
                   << ", part: " << req.partition_num();
      } else {
        impl::ReceiveQueueManager::Instance()->Put(req);
      }
    } else {
      ret = Err_Unsupported_op;
      LOG(ERROR) << "Unsupported repo type";
    }
    return ret;
  }

  static int32_t Exists(const proto::FileReq& req, proto::FileRes* res) {
    int32_t ret = Err_Success;
    if (!util::Util::Exists(req.dst())) {
      if (req.file_type() == proto::FileType::Direcotry) {
        util::Util::Mkdir(req.dst());
        return ret;
      }
      if (req.file_type() == proto::FileType::Symlink) {
        util::Util::CreateSymlink(req.dst(), req.content());
        return ret;
      }
    }

    if (req.file_type() == proto::FileType::Regular) {
      if (!std::filesystem::is_regular_file(req.dst())) {
        ret = Err_File_type_mismatch;
      }
    } else if (req.file_type() == proto::FileType::Direcotry) {
      if (!std::filesystem::is_directory(req.dst())) {
        ret = Err_File_type_mismatch;
      }
    } else if (req.file_type() == proto::FileType::Symlink) {
      if (!std::filesystem::is_symlink(req.dst())) {
        ret = Err_File_type_mismatch;
      }
    } else {
      LOG(ERROR) << "Unsupported file type";
    }

    auto update_time = util::Util::UpdateTime(req.dst());
    if (update_time != -1 && update_time == req.update_time()) {
      res->set_can_skip_upload(true);
    }

    return ret;
  }

  static int32_t Delete(const proto::FileReq& /*req*/,
                        proto::FileRes* /*res*/) {
    return Err_Success;
  }

  static void FileOpHandle(const proto::FileReq& req, proto::FileRes* res) {
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      if (req.token().empty()) {
        res->set_err_code(proto::ErrCode(Err_User_session_error));
        return;
      }

      std::string session_user;
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
    }

    int32_t ret = Err_Success;
    switch (req.op()) {
      case proto::FileOp::FilePut:
        ret = Save(req, res);
        break;
      case proto::FileOp::FileExists:
        ret = Exists(req, res);
        break;
      case proto::FileOp::FileDelete:
        ret = Delete(req, res);
        break;
      default:
        LOG(ERROR) << "Unsupported operation";
    }

    if (ret) {
      res->set_err_code(proto::ErrCode(ret));
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }

    res->set_src(req.src());
    res->set_dst(req.dst());
    res->set_file_hash(req.file_hash());
    res->set_partition_num(req.partition_num());
    res->set_file_type(req.file_type());
    res->set_request_id(req.request_id());
    res->set_op(req.op());
  }

  static void UserOpHandle(const proto::UserReq& req, proto::UserRes* res) {
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
