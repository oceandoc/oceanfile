/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/util/scan_manager.h"

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
  oceandoc::util::ScanContext ctx;
  ctx.src = path;
  ctx.status = &scan_status;

  LOG(INFO) << "Now validate: " << ctx.src;

  oceandoc::util::ScanManager::Instance()->ValidateScanStatus(&ctx);

  const int32_t max_threads = 4;
  for (const auto& d : ctx.status->scanned_dirs()) {
    auto hash = std::abs(oceandoc::util::Util::MurmurHash64A(d.first));
    if ((hash % max_threads) < 0 || (hash % max_threads) > 4) {
      LOG(ERROR) << d.first << ", " << (hash % max_threads);
    }
    for (const auto& f : d.second.files()) {
      auto hash = std::abs(oceandoc::util::Util::MurmurHash64A(f.first));
      if ((hash % max_threads) < 0 || (hash % max_threads) > 4) {
        LOG(ERROR) << d.first + "/" + f.first << ", " << (hash % max_threads);
      }
    }
  }
  return 0;
}
