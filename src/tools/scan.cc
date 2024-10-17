/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/impl/scan_manager.h"

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv, false);
  google::SetStderrLogging(google::GLOG_INFO);

  if (argc != 2) {
    LOG(ERROR) << "must have path arg";
    return -1;
  }

  oceandoc::util::ConfigManager::Instance()->Init("./conf/base_config.json");
  oceandoc::util::ThreadPool::Instance()->Init();

  std::string path = argv[1];

  oceandoc::proto::ScanStatus scan_status;
  oceandoc::common::ScanContext ctx;
  ctx.src = path;
  ctx.skip_scan = false;
  ctx.status = &scan_status;

  oceandoc::impl::ScanManager::Instance()->ParallelScan(&ctx);

  for (const auto& f : ctx.added_files) {
    LOG(INFO) << f << " added\n";
  }

  LOG(INFO) << "#######################################";
  for (const auto& f : ctx.removed_files) {
    LOG(INFO) << f << " removed\n";
  }

  return 0;
}
