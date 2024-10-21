/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H

#include <condition_variable>
#include <mutex>

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

  bool CreateRepo(const proto::RepoReq& req, proto::RepoRes* res) {
    grpc::ClientContext context;
    bool result;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
    stub_->async()->RepoOp(
        &context, &req, res,
        [&result, &mu, &cv, &done, res](grpc::Status status) {
          bool ret;
          if (!status.ok()) {
            ret = false;
          } else if (res->err_code() == proto::Success) {
            ret = true;
          } else {
            ret = false;
          }
          std::lock_guard<std::mutex> lock(mu);
          result = ret;
          done = true;
          cv.notify_one();
        });
    std::unique_lock<std::mutex> lock(mu);
    cv.wait(lock, [&done] { return done; });
    return result;
  }

  bool ListUserRepo(const std::string& user, const std::string& token) {
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

    for (const auto& p : res.repos()) {
      LOG(INFO) << "repo name: " << p.second.name();
    }

    LOG(INFO) << "ListUserRepo success, user: " << user;
    return true;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_REPO_CLIENT_H
