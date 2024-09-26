/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_SERVER_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_SERVER_HANDLER_H

#include "proxygen/httpserver/RequestHandler.h"

namespace oceandoc {
namespace server {
namespace http_handler {

class ServerHandler : public proxygen::RequestHandler {
 public:
  void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_SERVER_HANDLER_H
