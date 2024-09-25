/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "folly/init/Init.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
// #include "gperftools/profiler.h"

#if !defined(_WIN32)
#include <signal.h>
#endif

#include "src/common/module.h"
#include "src/server/grpc_server_impl.h"
#include "src/util/config_manager.h"
#include "src/util/repo_manager.h"
#include "src/util/scan_manager.h"
#include "src/util/thread_pool.h"

#if !defined(_WIN32)

// https://github.com/grpc/grpc/issues/24884
oceandoc::server::GrpcServer *server;
bool shutdown_required = false;
std::mutex mutex;
std::condition_variable cv;

void SignalHandler(int sig) {
  std::cout << "Got signal: " << strsignal(sig) << std::endl;
  shutdown_required = true;
  cv.notify_one();
}

void ShutdownCheckingThread(void) {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, []() { return shutdown_required; });
  server->Shutdown();
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
  LOG(INFO) << "Program initializing ...";

  folly::Init init(&argc, &argv, false);
  // google::InitGoogleLogging(argv[0]); // already called in folly::Init
  google::SetStderrLogging(google::GLOG_INFO);
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  oceandoc::common::InitAllModules(&argc, &argv);

  oceandoc::util::ConfigManager::Instance()->Init("./conf/base_config.json");
  oceandoc::util::ThreadPool::Instance()->Init();
  oceandoc::util::ScanManager::Instance()->Init();
  oceandoc::util::RepoManager::Instance()->Init();

#if !defined(_WIN32)
  RegisterSignalHandler();
  std::thread shutdown_thread(ShutdownCheckingThread);
#endif

  oceandoc::server::GrpcServer server;
  ::server = &server;
  LOG(INFO) << "CommandLine: " << google::GetArgv();

  server.WaitForShutdown();
  LOG(INFO) << "Now stopped grpc server";

#if !defined(_WIN32)
  if (shutdown_thread.joinable()) {
    shutdown_thread.join();
  }
#endif
  oceandoc::util::ThreadPool::Instance()->Stop();
  oceandoc::util::RepoManager::Instance()->Stop();

  // ProfilerStop();
  return 0;
}
