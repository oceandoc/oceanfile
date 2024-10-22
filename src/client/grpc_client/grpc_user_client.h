/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_USER_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_USER_CLIENT_H

#include <memory>

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/config_manager.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

using util::ConfigManager;
class UserClient {
 public:
  explicit UserClient(const std::string& addr, const std::string& port)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(proto::OceanFile::NewStub(channel_)) {
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    if (channel_->WaitForConnected(deadline)) {
      LOG(INFO) << "Connect to " << addr << ":" << port << " success";
    } else {
      LOG(INFO) << "Connect to " << addr << ":" << port << " error";
    }
  }

  bool Register(const std::string& user, const std::string& password,
                std::string* token) {
    proto::UserReq req;
    proto::UserRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(proto::UserOp::UserCreate);
    req.set_user(user);
    req.set_password(password);

    grpc::ClientContext context;
    auto status = stub_->UserOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    *token = res.token();
    LOG(INFO) << "Create success, token: " << *token;
    return true;
  }

  bool Login(const std::string& user, const std::string& password,
             std::string* token) {
    proto::UserReq req;
    proto::UserRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(proto::UserOp::UserLogin);
    req.set_user(user);
    req.set_password(password);

    grpc::ClientContext context;
    auto status = stub_->UserOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    *token = res.token();
    LOG(INFO) << "Login success, token: " << *token;
    return true;
  }

  bool ChangePassword(const std::string& user, const std::string& old_password,
                      const std::string& new_password,
                      const std::string& token) {
    proto::UserReq req;
    proto::UserRes res;
    req.set_request_id(util::Util::UUID());
    req.set_op(proto::UserOp::UserChangePassword);
    req.set_user(user);
    req.set_old_password(old_password);
    req.set_password(new_password);
    req.set_token(token);

    grpc::ClientContext context;
    auto status = stub_->UserOp(&context, req, &res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }

    if (res.err_code()) {
      LOG(ERROR) << "Server error: " << res.err_code();
      return false;
    }

    LOG(INFO) << "ChangePassword success, user: " << user;
    return true;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<proto::OceanFile::Stub> stub_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_USER_CLIENT_H
