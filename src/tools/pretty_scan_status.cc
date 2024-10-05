/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "folly/init/Init.h"
#include "glog/logging.h"
#include "src/util/scan_manager.h"
#include "src/util/util.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    LOG(ERROR) << "must have path arg";
    return -1;
  }

  std::string path = argv[1];

  folly::Init init(&argc, &argv, false);
  oceandoc::proto::ScanStatus scan_status;
  oceandoc::util::ScanContext ctx;
  ctx.status = &scan_status;
  oceandoc::util::ScanManager::Instance()->LoadCachedScanStatus(path, &ctx);
  std::string json;
  oceandoc::util::Util::MessageToPrettyJson(scan_status, &json);
  oceandoc::util::Util::WriteToFile("./data/pretty.json", json, false);

  json.clear();
  for (const auto& p : scan_status.scanned_files()) {
    json.append(p.first);
    json.append(":");
    json.append(std::to_string(p.second.update_time()));
    json.append("\n");
  }
  oceandoc::util::Util::WriteToFile("./data/file_list", json, false);

  json.clear();
  for (const auto& p : scan_status.scanned_dirs()) {
    json.append(p.first);
    json.append(":");
    json.append(std::to_string(p.second.update_time()));
    json.append("\n");
  }
  oceandoc::util::Util::WriteToFile("./data/dir_list", json, false);
  return 0;
}
