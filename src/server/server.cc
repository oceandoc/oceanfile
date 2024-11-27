/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "folly/init/Init.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "src/server/http_server_impl.h"
// #include "gperftools/profiler.h"

#if !defined(_WIN32)
#include <signal.h>
#endif

#include "src/impl/receive_queue_manager.h"
#include "src/impl/repo_manager.h"
#include "src/impl/scan_manager.h"
#include "src/server/grpc_server_impl.h"
#include "src/server/http_server_impl.h"
#include "src/server/server_context.h"
#include "src/util/config_manager.h"
#include "src/util/thread_pool.h"

#if !defined(_WIN32)
// https://github.com/grpc/grpc/issues/24884
oceandoc::server::GrpcServer *grpc_server_ptr = nullptr;
oceandoc::server::HttpServer *http_server_ptr = nullptr;
bool shutdown_required = false;
std::mutex mutex;
std::condition_variable cv;

void SignalHandler(int sig) {
  LOG(INFO) << "Got signal: " << strsignal(sig) << std::endl;
  shutdown_required = true;
  cv.notify_all();
}

void ShutdownCheckingThread(void) {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, []() { return shutdown_required; });
  grpc_server_ptr->Shutdown();
  http_server_ptr->Shutdown();
}

void RegisterSignalHandler() {
  signal(SIGTERM, &SignalHandler);
  signal(SIGINT, &SignalHandler);
  signal(SIGQUIT, &SignalHandler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
}
#endif

int main(int argc, char **argv) {
  // ProfilerStart("oceandoc_profile");
  LOG(INFO) << "Server initializing ...";
  std::string home_dir = oceandoc::util::Util::HomeDir();
  LOG(INFO) << "Home dir: " << home_dir;

  folly::Init init(&argc, &argv, false);
  // google::InitGoogleLogging(argv[0]); // already called in folly::Init
  google::SetStderrLogging(google::GLOG_INFO);
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "CommandLine: " << google::GetArgv();

  oceandoc::util::ConfigManager::Instance()->Init(
      home_dir + "/conf/server_base_config.json");
  oceandoc::util::ThreadPool::Instance()->Init();
  oceandoc::impl::ScanManager::Instance()->Init();
  oceandoc::impl::RepoManager::Instance()->Init();
  oceandoc::impl::ReceiveQueueManager::Instance()->Init();
  if (!oceandoc::impl::UserManager::Instance()->Init()) {
    LOG(ERROR) << "UserManager init error";
    return -1;
  }

#if !defined(_WIN32)
  RegisterSignalHandler();
  std::thread shutdown_thread(ShutdownCheckingThread);
#endif

  std::shared_ptr<oceandoc::server::ServerContext> server_context =
      std::make_shared<oceandoc::server::ServerContext>();

  oceandoc::server::GrpcServer grpc_server(server_context);
  oceandoc::server::HttpServer http_server(server_context);


#if !defined(_WIN32)
  ::grpc_server_ptr = &grpc_server;
  ::http_server_ptr = &http_server;
#endif

  grpc_server.Start();
  http_server.Start();

  grpc_server.WaitForShutdown();

  LOG(INFO) << "Now stopped grpc server";
  LOG(INFO) << "Now stopped http server";

#if !defined(_WIN32)
  if (shutdown_thread.joinable()) {
    shutdown_thread.join();
  }
#endif
  oceandoc::impl::UserManager::Instance()->Stop();
  oceandoc::impl::ReceiveQueueManager::Instance()->Stop();
  oceandoc::impl::RepoManager::Instance()->Stop();
  oceandoc::util::ThreadPool::Instance()->Stop();

  // ProfilerStop();
  return 0;
}
