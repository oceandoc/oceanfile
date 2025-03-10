/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_FILE_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_FILE_HANDLER_H

#include <filesystem>

#include "src/impl/repo_manager.h"
#include "src/impl/user_manager.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class FileHandler {
 public:
  static void FileOpFailResponse(proto::FileRes* res) {
    res->set_err_code(proto::ErrCode::Fail);
  }

  static int32_t Save(const proto::FileReq& req, proto::FileRes* /*res*/) {
    int32_t ret = Err_Success;
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      ret = impl::RepoManager::Instance()->WriteToFile(req);
    } else {
      ret = Err_Fail;
      LOG(ERROR) << "Unsupported repo type";
    }
    return ret;
  }

  static int32_t Read(const proto::FileReq& req, proto::FileRes* res) {
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      return impl::RepoManager::Instance()->ReadFile(req,
                                                     res->mutable_content());
    } else {
      LOG(ERROR) << "Unsupported repo type";
    }
    return Err_Fail;
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
        ret = Err_Fail;
      }
    } else if (req.file_type() == proto::FileType::Direcotry) {
      if (!std::filesystem::is_directory(req.dst())) {
        ret = Err_Fail;
      }
    } else if (req.file_type() == proto::FileType::Symlink) {
      if (!std::filesystem::is_symlink(req.dst())) {
        ret = Err_Fail;
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
      auto ret = impl::UserManager::Instance()->UserValidateSession(
          req.user(), req.token());
      if (ret) {
        res->set_err_code(proto::ErrCode(Err_User_session_error));
        return;
      }
    }

    res->set_src(req.src());
    res->set_dst(req.dst());
    res->set_file_hash(req.file_hash());
    res->set_partition_num(req.partition_num());
    res->set_file_type(req.file_type());
    res->set_request_id(req.request_id());
    res->set_op(req.op());

    int32_t ret = Err_Success;
    switch (req.op()) {
      case proto::FileOp::FilePut:
        ret = Save(req, res);
        break;
      case proto::FileOp::FileGet:
        ret = Read(req, res);
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
    LOG(INFO) << "res: " << util::Util::MessageToJson(*res);
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_FILE_HANDLER_H
