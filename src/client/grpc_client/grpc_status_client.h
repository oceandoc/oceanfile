/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_GRPC_STATUS_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_GRPC_STATUS_CLIENT_H

#include <condition_variable>
#include <mutex>
#include <vector>

#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/common/stucts.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"

using grpc::ClientContext;
using grpc::Status;

namespace oceandoc {
namespace client {

class StatusClient {
 public:
  explicit StatusClient(proto::OceanFile::Stub* stub) {
    // stub->async()->RepoOp(&context_, this);
  }

  void Reset() { done_ = false; }

  grpc::Status Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
  }

  bool Send() { return true; }

 private:
  ClientContext context_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::mutex write_mu_;
  std::condition_variable write_cv_;
  Status status_;
  bool done_ = false;
  proto::RepoReq req_;
  proto::RepoRes res_;
  mutable absl::base_internal::SpinLock lock_;
  std::vector<int32_t> mark_;
};
}  // namespace client
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_CLIENT_GRPC_STATUS_CLIENT_H
