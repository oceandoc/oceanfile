/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "glog/logging.h"
#include "src/util/util.h"

using oceandoc::util::Util;
using namespace std;

int main(int argc, char **argv) {
  if (argc != 3) {
    LOG(ERROR) << "must has two path args";
    return -1;
  }

  std::filesystem::path src = argv[1];
  std::filesystem::path dst = argv[2];
  string src_content;
  string dst_content;

  Util::LoadSmallFile(src.string(), &src_content);
  Util::LoadSmallFile(dst.string(), &dst_content);

  if (src_content.size() != dst_content.size()) {
    LOG(ERROR) << "file size different";
  }

  std::string_view sv(src_content.data() + 335544320, 10);
  LOG(INFO) << "content: " << Util::ToHexStr(sv);
  for (size_t i = 0; i < src_content.size() && i < dst_content.size(); ++i) {
    if (src_content[i] != dst_content[i]) {
      LOG(ERROR) << "pos: " << i << " different";
      return -1;
    }
  }
  return 0;
}
