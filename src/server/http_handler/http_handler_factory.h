/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_HANDLER_FACTORY_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_HANDLER_FACTORY_H

#include "proxygen/httpserver/RequestHandler.h"
#include "proxygen/httpserver/RequestHandlerFactory.h"
#include "src/server/http_handler/file_get_handler.h"
#include "src/server/http_handler/file_json_handler.h"
#include "src/server/http_handler/file_put_handler.h"
#include "src/server/http_handler/repo_handler.h"
#include "src/server/http_handler/server_handler.h"
#include "src/server/http_handler/user_handler.h"

namespace oceandoc {
namespace server {
namespace http_handler {

class HTTPHandlerFactory : public proxygen::RequestHandlerFactory {
 public:
  void onServerStart(folly::EventBase*) noexcept override {}
  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*,
      proxygen::HTTPMessage* message) noexcept override {
    if (message->getPath() == "/user") {
      return new UserHandler();
    } else if (message->getPath() == "/server") {
      return new ServerHandler();
    } else if (message->getPath() == "/file_put") {
      return new FilePutHandler();
    } else if (message->getPath() == "/file_get") {
      return new FileGetHandler();
    } else if (message->getPath() == "/file_json") {
      return new FileJsonHandler();
    } else if (message->getPath() == "/repo") {
      return new RepoHandler();
    }
    return nullptr;
  }
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_HANDLER_FACTORY_H
