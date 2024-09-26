/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H

#include "proxygen/httpserver/ResponseBuilder.h"

namespace oceandoc {
namespace server {
namespace http_handler {

class Util {
 public:
  static void InternalServerError(const std::string& res_body,
                                  proxygen::ResponseHandler* downstream) {
    proxygen::ResponseBuilder(downstream)
        .status(500, "Internal Server Error")
        .body(res_body)
        .sendWithEOM();
  }

  static void Success(const std::string& res_body,
                      proxygen::ResponseHandler* downstream) {
    proxygen::ResponseBuilder(downstream)
        .status(200, "Ok")
        .body(res_body)
        .sendWithEOM();
  }
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H
