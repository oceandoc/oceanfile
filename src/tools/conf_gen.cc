/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <iostream>

#include "src/tools/common.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "must has one path args";
    return -1;
  }

  std::filesystem::path origin_file = argv[1];
  std::string out;

  oceandoc::tools::Common::ConfGen(origin_file, &out);

  std::cout << out << std::endl;
  return 0;
}
