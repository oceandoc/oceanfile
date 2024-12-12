/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/impl/file_process_manager.h"

namespace oceandoc {
namespace impl {

static folly::Singleton<FileProcessManager> file_process_manager;

std::shared_ptr<FileProcessManager> FileProcessManager::Instance() {
  return file_process_manager.try_get();
}

}  // namespace impl
}  // namespace oceandoc
