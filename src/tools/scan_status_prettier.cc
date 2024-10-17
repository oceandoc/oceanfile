/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/impl/scan_manager.h"
#include "src/util/util.h"

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
  ctx.status = &scan_status;

  LOG(INFO) << "Now pretty: " << ctx.src;
  oceandoc::impl::ScanManager::Instance()->LoadCachedScanStatus(&ctx);
  std::string json;
  oceandoc::util::Util::MessageToPrettyJson(scan_status, &json);
  oceandoc::util::Util::WriteToFile("./data/pretty.json", json, false);

  std::string file_list;
  std::string dir_list;

  file_list.reserve(64 * 1024 * 1024 * 8);
  dir_list.reserve(64 * 1024 * 1024 * 8);

  for (const auto& d : scan_status.scanned_dirs()) {
    dir_list.append(d.first);
    dir_list.append(":");
    dir_list.append(std::to_string(d.second.update_time()));
    dir_list.append("\n");

    for (const auto& f : d.second.files()) {
      file_list.append(d.first + "/" + f.first);
      file_list.append(":");
      file_list.append(std::to_string(f.second.update_time()));
      file_list.append("\n");
    }
  }
  oceandoc::util::Util::WriteToFile("./data/dir_list", dir_list, false);
  oceandoc::util::Util::WriteToFile("./data/file_list", file_list, false);
  return 0;
}
