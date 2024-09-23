/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H

#include "glog/logging.h"
#include "src/client/grpc_client/grpc_file_client.h"
#include "src/client/grpc_client/grpc_repo_client.h"
#include "src/client/grpc_client/grpc_status_client.h"
#include "src/util/util.h"

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);
  LOG(INFO) << "Program initializing ...";
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  oceandoc::client::StatusClient status_client("localhost", "10001");
  oceandoc::proto::StatusReq status_req;
  oceandoc::proto::StatusRes status_res;
  status_req.set_request_id(oceandoc::util::Util::UUID());
  if (status_client.Status(status_req, &status_res)) {
    LOG(INFO) << "Server status: \n" << status_res.status();
  } else {
    LOG(ERROR) << "Get server status error";
    return -1;
  }

  oceandoc::client::RepoClient repo_client("localhost", "10001");
  oceandoc::proto::RepoReq repo_req;
  oceandoc::proto::RepoRes repo_res;
  status_req.set_request_id(oceandoc::util::Util::UUID());
  repo_req.set_op(oceandoc::proto::Op::Repo_Create);
  repo_req.set_path("/tmp/test_repo");
  if (repo_client.CreateRepo(repo_req, &repo_res)) {
    LOG(INFO) << "Create repo success, path: " << repo_req.path()
              << ", uuid: " << repo_res.repo_uuid();
  } else {
    LOG(ERROR) << "Create repo error";
    return -1;
  }

  std::string path =
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus";
  oceandoc::client::FileClient file_client("localhost", "10001");
  auto ret = file_client.Send(repo_res.repo_uuid(), path);
  grpc::Status status = file_client.Await();
  if (!status.ok() || !ret) {
    LOG(ERROR) << "Store " << path << " failed.";
  }
  return 0;
}

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_CLIENT_H
