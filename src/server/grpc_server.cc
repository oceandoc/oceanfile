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
oceandoc::server::GrpcServer *server;

void SignalHandler(int sig) { server->SignalHandler(sig); }

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

  oceandoc::server::GrpcServer server;
  LOG(INFO) << "CommandLine: " << google::GetArgv();

#if !defined(_WIN32)
  ::server = &server;
  RegisterSignalHandler();
#endif

  server.WaitForShutdown();
  LOG(INFO) << "Now stop grpc server";

  oceandoc::util::ThreadPool::Instance()->Stop();
  oceandoc::util::RepoManager::Instance()->Stop();

  // ProfilerStop();
  return 0;
}
