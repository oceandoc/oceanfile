/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_PUT_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_PUT_HANDLER_H

#include "folly/io/IOBuf.h"
#include "proxygen/httpserver/RequestHandler.h"
#include "src/common/defs.h"
#include "src/server/handler_proxy/handler_proxy.h"
#include "src/server/http_handler/util.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace http_handler {

// https://tonybai.com/2021/01/16/upload-and-download-file-using-multipart-form-over-http/
class FilePutHandler : public proxygen::RequestHandler {
 public:
  FilePutHandler() { body_.reserve(common::NET_BUFFER_SIZE_BYTES + 100); }

  void onUpgrade(proxygen::UpgradeProtocol) noexcept override {}
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
    const std::string& contentType =
        msg->getHeaders().getSingleOrEmpty("Content-Type");
    boundary_ = contentType.substr(30);
  }

  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
    if (body) {
      body_.append(reinterpret_cast<const char*>(body->data()), body->length());
    }
  }

  void onEOM() noexcept override {
    proto::FileReq req;
    proto::FileRes res;

    std::string res_body = "Parse request error";
    if (!Util::HandleMultipart(body_, boundary_, &req)) {
      Util::InternalServerError(res_body, downstream_);
      return;
    }

    handler_proxy::HandlerProxy::FileOpHandle(req, &res);

    res_body.clear();
    if (!util::Util::MessageToJson(res, &res_body)) {
      // if (!res.SerializeToString(&res_body)) {
      res_body = "Res pb to json error";
      Util::InternalServerError(res_body, downstream_);
      return;
    }
    Util::Success(res_body, downstream_);
  }

  void requestComplete() noexcept override { delete this; }

  void onError(proxygen::ProxygenError err) noexcept override {
    LOG(ERROR) << "error: " << proxygen::getErrorString(err);
    delete this;
  }

 private:
  std::string body_;
  std::string boundary_;
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_PUT_HANDLER_H
