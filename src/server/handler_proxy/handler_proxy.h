/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
#define BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H

#include <filesystem>

#include "src/proto/service.pb.h"
#include "src/util/receive_queue_manager.h"
#include "src/util/repo_manager.h"
#include "src/util/sync_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace handler_proxy {

class HandlerProxy {
 public:
  static void FileOpFailResponse(proto::FileRes* res) {
    res->set_err_code(proto::ErrCode::Fail);
  }

  static int32_t SaveFile(const proto::FileReq& req, proto::FileRes* res) {
    int32_t ret = Err_Success;
    if (req.repo_type() == proto::RepoType::RT_Ocean) {
      if (req.repo_uuid().empty()) {
        ret = Err_Repo_uuid_error;
        LOG(ERROR) << "Repo uuid empty";
      } else {
        ret = util::RepoManager::Instance()->WriteToFile(req);
        util::ReceiveQueueManager::Instance()->Put(req);
      }
    } else if (req.repo_type() == proto::RepoType::RT_Remote) {
      ret = util::SyncManager::Instance()->WriteToFile(req);
      util::ReceiveQueueManager::Instance()->Put(req);
    } else {
      LOG(ERROR) << "Unsupported repo type";
    }
    return ret;
  }

  static int32_t Exists(const proto::FileReq& req, proto::FileRes* res) {
    int32_t ret = Err_Success;
    if (!util::Util::Exists(req.path())) {
    }
    if (req.file_type() == proto::FileType::Regular) {
      if (!std::filesystem::is_regular_file(req.path())) {
        ret = Err_File_type_mismatch;
      }
    } else if (req.file_type() == proto::FileType::Dir) {
      if (!std::filesystem::is_directory(req.path())) {
        ret = Err_File_type_mismatch;
      }
    } else if (req.file_type() == proto::FileType::Symlink) {
      if (!std::filesystem::is_symlink(req.path())) {
        ret = Err_File_type_mismatch;
      }
    } else {
      LOG(ERROR) << "Unsupported file type";
    }
    return ret;
  }

  static void FileOpHandle(const proto::FileReq& req, proto::FileRes* res) {
    int32_t ret = true;
    switch (req.op()) {
      case proto::FileOp::FilePut:
        ret = SaveFile(req, res);
        break;
      case proto::FileOp::FileExists:
        ret = Exists(req, res);
        break;
      default:
        LOG(ERROR) << "Unsupported operation";
    }

    if (ret) {
      LOG(ERROR) << "Store file error, hash: " << req.hash()
                 << ", path: " << req.path()
                 << ", part: " << req.partition_num();
      res->set_err_code(proto::ErrCode(ret));
    } else {
      res->set_err_code(proto::ErrCode::Success);
    }

    res->set_path(req.path());
    res->set_hash(req.hash());
    res->set_partition_num(req.partition_num());
    res->set_file_type(req.file_type());
  }
};

}  // namespace handler_proxy
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HANDLER_PROXY_H
