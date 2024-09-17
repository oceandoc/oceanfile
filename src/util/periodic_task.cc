/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/periodic_task.h"

namespace oceandoc {
namespace util {

static folly::Singleton<PeriodicTask> periodic_task;

std::shared_ptr<PeriodicTask> PeriodicTask::Instance() {
  return periodic_task.try_get();
}

}  // namespace util
}  // namespace oceandoc
