/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_META_H
#define BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_META_H

#include "src/async_grpc/type_traits.h"
#include "src/proto/service.pb.h"

namespace oceandoc {
namespace server {
namespace grpc_handler {

struct StatusMethod {
  static constexpr const char* MethodName() {
    return "/oceandoc.proto.OceanFile/Status";
  }
  using IncomingType = oceandoc::proto::StatusReq;
  using OutgoingType = oceandoc::proto::StatusRes;
};

struct RepoOpMethod {
  static constexpr const char* MethodName() {
    return "/oceandoc.proto.OceanFile/RepoOp";
  }
  using IncomingType = oceandoc::proto::RepoReq;
  using OutgoingType = oceandoc::proto::RepoRes;
};

struct FileOpMethod {
  static constexpr const char* MethodName() {
    return "/oceandoc.proto.OceanFile/FileOp";
  }
  using IncomingType = async_grpc::Stream<oceandoc::proto::FileReq>;
  using OutgoingType = async_grpc::Stream<oceandoc::proto::FileRes>;
};

}  // namespace grpc_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_GRPC_HANDLERS_META_H
