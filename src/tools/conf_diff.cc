/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <iostream>

#include "src/tools/common.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "must has two path args";
    return -1;
  }

  std::filesystem::path origin_file = argv[1];
  std::filesystem::path final_file = argv[2];
  std::string out;

  oceandoc::tools::Common::ConfDiff(origin_file, final_file, &out);

  std::cout << out << std::endl;
  return 0;
}
