/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

class RepoClient {
 public:
  explicit RepoClient(const std::string& addr, const std::string& port)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(oceandoc::proto::OceanFile::NewStub(channel_)) {}

  bool ListUserRepo(
      const std::string& user, const std::string& token,
      google::protobuf::Map<std::string, proto::RepoMeta>* repos) {
    oceandoc::proto::RepoReq req;
    oceandoc::proto::RepoRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(oceandoc::proto::RepoOp::RepoListUserRepo);
    req.set_user(user);
    req.set_token(token);

    grpc::ClientContext context;
    auto status = stub_->RepoOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    repos->swap(*res.mutable_repos());
    LOG(INFO) << "ListUserRepo success, user: " << user
              << ", num: " << repos->size();
    return true;
  }

  bool ListServerDir(const std::string& user, const std::string& token,
                     const std::string& path, proto::DirItem* dir) {
    oceandoc::proto::RepoReq req;
    oceandoc::proto::RepoRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(oceandoc::proto::RepoOp::RepoListServerDir);
    req.set_user(user);
    req.set_token(token);
    req.set_path(path);

    grpc::ClientContext context;
    auto status = stub_->RepoOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    LOG(INFO) << "ListServerDir success, path: " << (path.empty() ? "/" : path);
    dir->Swap(res.mutable_dir());
    return true;
  }

  bool CreateServerDir(const std::string& user, const std::string& token,
                       const std::string& path) {
    oceandoc::proto::RepoReq req;
    oceandoc::proto::RepoRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(oceandoc::proto::RepoOp::RepoCreateServerDir);
    req.set_user(user);
    req.set_token(token);
    req.set_path(path);

    grpc::ClientContext context;
    auto status = stub_->RepoOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    LOG(INFO) << "CreateServerDir success, path: " << path;
    return true;
  }

  bool CreateRepo(const std::string& user, const std::string& token,
                  const std::string& repo_name, const std::string& path,
                  proto::RepoMeta* repo) {
    oceandoc::proto::RepoReq req;
    oceandoc::proto::RepoRes res;
    grpc::ClientContext context;

    req.set_request_id(util::Util::UUID());
    req.set_op(proto::RepoOp::RepoCreateRepo);
    req.set_user(user);
    req.set_token(token);
    req.set_repo_name(repo_name);
    req.set_path(path);

    auto status = stub_->RepoOp(&context, req, &res);

    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    LOG(INFO) << "Create reop: " << repo_name << " success";
    repo->Swap(res.mutable_repo());
    return true;
  }

  bool DeleteRepo(const std::string& user, const std::string& token,
                  const std::string& repo_uuid) {
    oceandoc::proto::RepoReq req;
    oceandoc::proto::RepoRes res;
    grpc::ClientContext context;

    req.set_request_id(util::Util::UUID());
    req.set_op(proto::RepoOp::RepoDeleteRepo);
    req.set_user(user);
    req.set_token(token);
    req.set_repo_uuid(repo_uuid);

    auto status = stub_->RepoOp(&context, req, &res);

    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    LOG(INFO) << "Delete reop: " << repo_uuid << " success";
    return true;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H
