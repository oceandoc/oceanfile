/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/client/grpc_client/grpc_file_client.h"

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "src/client/grpc_client/grpc_user_client.h"
#include "src/util/config_manager.h"
#include "src/util/thread_pool.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

TEST(FileClient, Send) {
  std::string home_dir = util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;
  util::ConfigManager::Instance()->Init(home_dir +
                                        "/conf/client_base_config.json");
  util::ThreadPool::Instance()->Init();
  auto addr = util::ConfigManager::Instance()->ServerAddr();
  auto port = std::to_string(util::ConfigManager::Instance()->GrpcServerPort());
  std::string user = "dev";
  std::string password = "just_for_test";
  std::string token;

  UserClient user_client(addr, port);
  if (user_client.Login("dev", password, &token)) {
    LOG(INFO) << "Login dev success";
  }

  common::SyncContext sync_ctx(4);
  sync_ctx.hash_method = common::HashMethod::Hash_BLAKE3;
  sync_ctx.repo_type = proto::RepoType::RT_Ocean;
  sync_ctx.partition_size = common::NET_BUFFER_SIZE_BYTES;
  sync_ctx.remote_addr = "127.0.0.1";
  sync_ctx.remote_port = "10001";
  sync_ctx.user = "dev";
  sync_ctx.token = token;
  sync_ctx.repo_uuid = "7c602a90-3693-46d0-b80c-221fd1737a66";

  FileClient file_client(&sync_ctx);
  file_client.Start();

  std::string path =
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus";
  std::shared_ptr<common::SendContext> send_ctx =
      std::make_shared<common::SendContext>();
  send_ctx->src = path;
  send_ctx->op = proto::FileOp::FilePut;
  send_ctx->file_type = proto::FileType::Regular;

  file_client.Put(send_ctx);
  file_client.SetFillQueueComplete();
  file_client.Await();
}

}  // namespace client
}  // namespace oceandoc
