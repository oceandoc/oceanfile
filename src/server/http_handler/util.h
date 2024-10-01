/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H
#define BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H

#include "MultipartReader.h"
#include "proxygen/httpserver/ResponseBuilder.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {
namespace http_handler {

enum MultiPartField {
  MP_unknown = 0,
  MP_op,
  MP_path,
  MP_sha256,
  MP_size,
  MP_content,
  MP_partition_num,
  MP_repo_uuid,
  MP_partition_size,
};

struct MultiContext {
  MultiContext(proto::FileReq* req) : req(req) {}

  MultiPartField field;
  std::string_view data;
  proto::FileReq* req;
};

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

  static std::string GetName(const std::string& value) {
    // LOG(INFO) << value;
    auto start = value.find_first_of("\"");
    if (start == std::string::npos) {
      return "";
    }

    auto end = value.find_first_of("\"", start + 1);
    if (end == std::string::npos) {
      return "";
    }
    return value.substr(start + 1, end - start - 1);
  }

  static void FillFileReq(const MultiContext& context) {
    switch (context.field) {
      case MP_op:
        context.req->set_op(
            proto::FileOp(util::Util::ToInt<int32_t>(context.data)));
        break;
      case MP_path:
        context.req->set_path(context.data);
        break;
      case MP_sha256:
        context.req->set_sha256(context.data);
        break;
      case MP_size:
        context.req->set_size(util::Util::ToInt<int64_t>(context.data));
        break;
      case MP_content:
        context.req->mutable_content()->append(context.data);
        break;
      case MP_partition_num:
        context.req->set_partition_num(
            util::Util::ToInt<int32_t>(context.data));
        break;
      case MP_repo_uuid:
        context.req->set_repo_uuid(context.data);
        break;
      case MP_partition_size:
        context.req->set_partition_size(
            util::Util::ToInt<int64_t>(context.data));
        if (context.req->content().empty()) {
          context.req->mutable_content()->reserve(
              context.req->partition_size());
        }
        break;
      default:
        // TODO reject IP for a moment
        LOG(ERROR) << "Unknow filed";
    }
  }

  static bool HandleMultipart(const std::string& body,
                              const std::string& boundary,
                              proto::FileReq* req) {
    MultiContext context(req);
    MultipartReader reader(boundary);
    reader.userData = &context;
    reader.onPartBegin = [](const MultipartHeaders& current_headers,
                            void* userData) {
      if (current_headers.size() <= 0) {
        return;
      }

      MultiContext* context = static_cast<MultiContext*>(userData);
      auto it = current_headers.find("Content-Disposition");
      if (it == current_headers.end()) {
        LOG(ERROR) << "Cannot find Content-Disposition";
        return;
      }
      // form-data; name="chunkIndex"
      auto name = GetName(it->second);
      if (name == "op") {
        context->field = MultiPartField::MP_op;
      } else if (name == "path") {
        context->field = MultiPartField::MP_path;
      } else if (name == "sha256") {
        context->field = MultiPartField::MP_sha256;
      } else if (name == "size") {
        context->field = MultiPartField::MP_size;
      } else if (name == "content") {
        context->field = MultiPartField::MP_content;
      } else if (name == "partition_num") {
        context->field = MultiPartField::MP_partition_num;
      } else if (name == "repo_uuid") {
        context->field = MultiPartField::MP_repo_uuid;
      } else if (name == "repo_uuid") {
        context->field = MultiPartField::MP_repo_uuid;
      } else if (name == "partition_size") {
        context->field = MultiPartField::MP_partition_size;
      } else {
        context->field = MultiPartField::MP_unknown;
      }
    };

    reader.onPartData = [](const char* data, size_t size, void* userData) {
      MultiContext* context = static_cast<MultiContext*>(userData);
      context->data = std::string_view(data, size);
      FillFileReq(*context);
    };

    size_t pos = 0;
    do {
      pos = reader.feed(body.data(), body.size());
      LOG(INFO) << "pos: " << pos;
    } while (reader.succeeded() && pos < body.size());
    if (reader.hasError()) {
      LOG(INFO) << reader.getErrorMessage();
    }

    return !reader.hasError();
  }
};

}  // namespace http_handler
}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_HTTP_HANDLER_UTIL_H
