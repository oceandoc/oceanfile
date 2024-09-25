/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_SERVER_IMPL_H
#define BAZEL_TEMPLATE_SERVER_GRPC_SERVER_IMPL_H

#include <memory>
#include <string>

#include "absl/strings/str_cat.h"
#include "src/async_grpc/server.h"
#include "src/server/grpc_handler/file_handler.h"
#include "src/server/grpc_handler/repo_handler.h"
#include "src/server/grpc_handler/status_handler.h"
#include "src/server/server_context.h"
#include "src/util/config_manager.h"

namespace oceandoc {
namespace server {

class GrpcServer final {
 public:
  GrpcServer() : terminated(false) {
    async_grpc::Server::Builder server_builder;
    std::string addr_port =
        util::ConfigManager::Instance()->ServerAddr() + ":" +
        absl::StrCat(util::ConfigManager::Instance()->GrpcServerPort());
    server_builder.SetServerAddress(addr_port);
    server_builder.SetNumGrpcThreads(
        util::ConfigManager::Instance()->GrpcThreads());
    server_builder.SetNumEventThreads(
        util::ConfigManager::Instance()->EventThreads());

    server_builder.RegisterHandler<grpc_handler::StatusHandler>();
    server_builder.RegisterHandler<grpc_handler::RepoHandler>();
    server_builder.RegisterHandler<grpc_handler::FileHandler>();

    server_ = server_builder.Build();

    server_->SetExecutionContext(std::make_shared<ServerContext>());
    server_->Start();

    server_->GetContext<ServerContext>()->MarkedServerInitedDone();
  }

 public:
  void WaitForShutdown() { server_->WaitForShutdown(); }

  void Shutdown() { server_->Shutdown(); }

  std::shared_ptr<async_grpc::Server> GetGrpcServer() { return server_; }

 private:
  std::shared_ptr<async_grpc::Server> server_;

 public:
  std::atomic_bool terminated;
};

}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_SERVER_IMPL_H
