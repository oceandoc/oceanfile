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

#include "proxygen/httpserver/HTTPServer.h"
#include "proxygen/httpserver/HTTPServerOptions.h"
#include "src/common/module.h"
#include "src/server/http_handler/http_handler_factory.h"
#include "src/util/config_manager.h"
#include "src/util/repo_manager.h"
#include "src/util/scan_manager.h"
#include "src/util/thread_pool.h"

#if !defined(_WIN32)
// https://github.com/grpc/grpc/issues/24884
proxygen::HTTPServer* server;
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
  server->stop();
}

void RegisterSignalHandler() {
  signal(SIGTERM, &SignalHandler);
  signal(SIGINT, &SignalHandler);
  signal(SIGQUIT, &SignalHandler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
}
#endif

int main(int argc, char* argv[]) {
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

  proxygen::HTTPServerOptions options;
  options.threads = static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN));
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.handlerFactories =
      proxygen::RequestHandlerChain()
          .addThen<oceandoc::server::http_handler::HTTPHandlerFactory>()
          .build();

  std::string addr = oceandoc::util::ConfigManager::Instance()->ServerAddr();
  int32_t port = oceandoc::util::ConfigManager::Instance()->HttpServerPort();

  std::vector<proxygen::HTTPServer::IPConfig> IPs = {
      {folly::SocketAddress(addr, port, true),
       proxygen::HTTPServer::Protocol::HTTP},
      {folly::SocketAddress(addr, port, true),
       proxygen::HTTPServer::Protocol::HTTP2}};

  proxygen::HTTPServer server(std::move(options));
  server.bind(IPs);

  std::thread t([&]() { server.start(); });

  t.join();

#if !defined(_WIN32)
  if (shutdown_thread.joinable()) {
    shutdown_thread.join();
  }
#endif

  oceandoc::util::ThreadPool::Instance()->Stop();
  oceandoc::util::RepoManager::Instance()->Stop();

  return 0;
}
