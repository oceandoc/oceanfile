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

#include "src/server/grpc_server_impl.h"
#include "src/server/http_server_impl.h"
#include "src/server/server_context.h"
#include "src/util/config_manager.h"
#include "src/util/receive_queue_manager.h"
#include "src/util/repo_manager.h"
#include "src/util/scan_manager.h"
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
  LOG(INFO) << "Grpc server initializing ...";

  folly::Init init(&argc, &argv, false);
  // google::InitGoogleLogging(argv[0]); // already called in folly::Init
  google::SetStderrLogging(google::GLOG_INFO);
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "CommandLine: " << google::GetArgv();

  oceandoc::util::ConfigManager::Instance()->Init("./conf/base_config.json");
  oceandoc::util::ThreadPool::Instance()->Init();
  oceandoc::util::ScanManager::Instance()->Init();
  oceandoc::util::RepoManager::Instance()->Init();
  oceandoc::util::ReceiveQueueManager::Instance()->Init();

#if !defined(_WIN32)
  RegisterSignalHandler();
  std::thread shutdown_thread(ShutdownCheckingThread);
#endif

  std::shared_ptr<oceandoc::server::ServerContext> server_context =
      std::make_shared<oceandoc::server::ServerContext>();

  oceandoc::server::GrpcServer grpc_server(server_context);
  ::grpc_server_ptr = &grpc_server;

  oceandoc::server::HttpServer http_server(server_context);
  ::http_server_ptr = &http_server;

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
  oceandoc::util::ReceiveQueueManager::Instance()->Stop();
  oceandoc::util::RepoManager::Instance()->Stop();
  oceandoc::util::ThreadPool::Instance()->Stop();

  // ProfilerStop();
  return 0;
}
