/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include "glog/logging.h"
#include "src/impl/repo_manager.h"
#include "src/impl/user_manager.h"
#include "src/server/http_handler/util.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {

class FileGetHandler : public proxygen::RequestHandler {
 public:
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override {
    // Get authorization token from header
    auto auth_header = headers->getHeaders().getSingleOrEmpty("Authorization");
    if (auth_header.empty()) {
      LOG(ERROR) << "Missing Authorization header";
      proxygen::ResponseBuilder(downstream_)
          .status(401, "Unauthorized")
          .body("Authorization required")
          .sendWithEOM();
      return;
    }

    // Validate token
    std::string token = auth_header;
    if (!impl::UserManager::Instance()->ValidateToken(token)) {
      LOG(ERROR) << "Invalid authorization token";
      proxygen::ResponseBuilder(downstream_)
          .status(403, "Forbidden")
          .body("Invalid authorization")
          .sendWithEOM();
      return;
    }

    // Parse request parameters
    auto params = util::ParseQueryString(headers->getQueryString());
    
    // Required parameters
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

    // Read file using repo manager
    auto ret = impl::RepoManager::Instance()->ReadFile(file_req);
    if (ret != Err_Success) {
      LOG(ERROR) << "Failed to read file: " << ret;
      proxygen::ResponseBuilder(downstream_)
          .status(404, "Not Found")
          .body("File not found or error reading file")
          .sendWithEOM();
      return;
    }

    // Set response headers
    proxygen::ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", "application/octet-stream")
        .header("Content-Disposition", 
                "attachment; filename=\"" + file_req.file_hash() + "\"")
        .send();

    // Send file content in chunks
    // Note: Implement chunked transfer if needed for large files
    std::string content;
    if (util::Util::LoadSmallFile(file_req.file_hash(), &content)) {
      downstream_->sendBody(std::move(content));
    }

    downstream_->sendEOM();
  }

  void onBody(std::unique_ptr<folly::IOBuf> /*body*/) noexcept override {}
  void onEOM() noexcept override {}
  void onUpgrade(proxygen::UpgradeProtocol /*protocol*/) noexcept override {}

  void requestComplete() noexcept override { delete this; }
  void onError(proxygen::ProxygenError /*err*/) noexcept override { delete this; }
};

}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_FILE_GET_HANDLER_H
