/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <memory>
#include <string>

#include "absl/strings/str_cat.h"
#include "src/async_grpc/server.h"
#include "src/server/grpc_handler/put_file_handler.h"
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

    server_builder
        .RegisterHandler<oceandoc::server::grpc_handler::PutFileHandler>();
    server_ = server_builder.Build();
    server_->SetExecutionContext(std::make_shared<ServerContext>());
    server_->Start();

    server_->GetContext<ServerContext>()->MarkedServerInitedDone();
  }

 public:
  void WaitForShutdown() { server_->WaitForShutdown(); }

  std::shared_ptr<async_grpc::Server> GetGrpcServer() { return server_; }

 private:
  std::shared_ptr<async_grpc::Server> server_;

 public:
  std::atomic_bool terminated;
};

}  // namespace server
}  // namespace oceandoc
