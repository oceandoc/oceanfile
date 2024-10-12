/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/receive_queue_manager.h"

namespace oceandoc {
namespace util {

static folly::Singleton<ReceiveQueueManager> receive_queue_manager;

std::shared_ptr<ReceiveQueueManager> ReceiveQueueManager::Instance() {
  return receive_queue_manager.try_get();
}

}  // namespace util
}  // namespace oceandoc
