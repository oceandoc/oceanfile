/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H

#include "glog/logging.h"
#include "proxygen/httpserver/RequestHandler.h"
#include "src/impl/repo_manager.h"
#include "src/impl/user_manager.h"
#include "src/server/http_handler/util.h"

namespace oceandoc {
namespace server {
namespace http_handler {

class FileGetHandler : public proxygen::RequestHandler {
 public:
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
    auto full_url = Util::GetFullUrl(msg.get());
    auto params = Util::ParseQueryString(full_url);

    if (!params.count("user") || !params.count("token")) {
      proxygen::ResponseBuilder(downstream_)
          .status(401, "Unauthorized")
          .body("Authorization required")
          .sendWithEOM();
      return;
    }

    const std::string& user = params["user"];
    const std::string& token = params["token"];
    if (impl::UserManager::Instance()->UserValidateSession(user, token)) {
      LOG(ERROR) << "Invalid authorization token";
      LOG(INFO) << impl::UserManager::Instance()->UserQueryToken(user);
      proxygen::ResponseBuilder(downstream_)
          .status(403, "Forbidden")
          .body("Invalid authorization")
          .sendWithEOM();
      return;
    }

    if (!params.count("repo_uuid") || !params.count("file_hash")) {
      LOG(ERROR) << "Missing required parameters";
      proxygen::ResponseBuilder(downstream_)
          .status(400, "Bad Request")
          .body("Missing required parameters")
          .sendWithEOM();
      return;
    }

    proto::FileReq file_req;
    file_req.set_repo_uuid(params["repo_uuid"]);
    file_req.set_file_hash(params["file_hash"]);

    // Set response headers

    std::string content;
    if (!impl::RepoManager::Instance()->ReadFile(file_req, &content)) {
      proxygen::ResponseBuilder(downstream_)
          .status(200, "OK")
          .header("Content-Type", "image/jpeg")
          .header("Content-Disposition",
                  "attachment; filename=\"" + file_req.file_hash() + ".jpeg\"")
          .body(content)
          .send();
    } else {
      LOG(ERROR) << "Failed to read file: " << file_req.file_hash()
                 << ", repo: " << file_req.repo_uuid();
      proxygen::ResponseBuilder(downstream_)
          .status(404, "Not Found")
          .body("File not found or error reading file")
          .sendWithEOM();
    }
  }

  void onBody(std::unique_ptr<folly::IOBuf> /*body*/) noexcept override {}
  void onEOM() noexcept override {}
  void onUpgrade(proxygen::UpgradeProtocol /*protocol*/) noexcept override {}

  void requestComplete() noexcept override { delete this; }
  void onError(proxygen::ProxygenError /*err*/) noexcept override {
    delete this;
  }
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H
