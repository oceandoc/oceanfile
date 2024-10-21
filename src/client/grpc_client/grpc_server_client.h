/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_SERVER_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_SERVER_CLIENT_H

#include <memory>

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"

namespace oceandoc {
namespace client {

class ServerClient {
 public:
  explicit ServerClient(const std::string& addr, const std::string& port)
      : channel_(grpc::CreateChannel(addr + ":" + port,
                                     grpc::InsecureChannelCredentials())),
        stub_(oceandoc::proto::OceanFile::NewStub(channel_)) {}

  bool Status(const proto::ServerReq& req, proto::ServerRes* res) {
    grpc::ClientContext context;
    auto status = stub_->ServerOp(&context, req, res);
    if (!status.ok()) {
      LOG(ERROR) << "Grpc error";
      return false;
    }
    return true;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub_;
};

}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_SERVER_CLIENT_H
