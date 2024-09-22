/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/client/grpc_client/grpc_file_client.h"

using grpc::ClientContext;
using grpc::Status;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);
  LOG(INFO) << "Program initializing ...";
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  std::string path =
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus";

  auto channel = grpc::CreateChannel("localhost:10001",
                                     grpc::InsecureChannelCredentials());
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub(
      oceandoc::proto::OceanFile::NewStub(channel));
  oceandoc::client::FileClient file_client(stub.get());

  file_client.Send(path);

  Status status = file_client.Await();
  if (!status.ok()) {
    LOG(ERROR) << "Math rpc failed.";
  }
  return 0;
}

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H
