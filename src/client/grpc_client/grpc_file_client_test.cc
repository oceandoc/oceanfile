/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/client/grpc_client/grpc_file_client.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace client {

TEST(FileClient, Send) {
  std::string path =
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus";
  oceandoc::client::FileClient file_client(
      "192.168.4.100", "10001", oceandoc::proto::RepoType::RT_Remote);
  oceandoc::client::SendContext send_ctx;
  send_ctx.src = path;
  send_ctx.dst =
      "/tmp/test_dir/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus";
  auto ret = file_client.Send(send_ctx);
  grpc::Status status = file_client.Await();
  if (!status.ok() || !ret) {
    LOG(ERROR) << "Store " << path << " failed.";
  }
}

}  // namespace client
}  // namespace oceandoc
