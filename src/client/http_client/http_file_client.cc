/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_CLIENT_HTTP_FILE_CLIENT_H
#define BAZEL_TEMPLATE_CLIENT_HTTP_FILE_CLIENT_H

#include <fstream>

#include "curl/curl.h"
#include "glog/logging.h"
#include "src/common/defs.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

class FileClient {};

}  // namespace client
}  // namespace oceandoc

struct UploadContext {
  std::ifstream file;
  size_t remaining;
};

size_t readCallback(void* ptr, size_t size, size_t nmemb, void* stream) {
  UploadContext* context = static_cast<UploadContext*>(stream);
  if (!context->file.is_open()) {
    return CURL_READFUNC_ABORT;
  }

  context->file.read(static_cast<char*>(ptr), size * nmemb);
  size_t bytesRead = context->file.gcount();
  context->remaining -= bytesRead;

  return bytesRead;
}

int main() {
  CURL* curl;
  CURLcode res;

  const char* filePath = "path/to/your/file";
  const char* url = "http://example.com/upload";

  std::ifstream file(filePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return 1;
  }

  size_t fileSize = file.tellg();
  file.seekg(0);

  UploadContext context{std::move(file), fileSize};

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &context);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     static_cast<curl_off_t>(context.remaining));

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                << std::endl;
    }

    curl_easy_cleanup(curl);
  }

  return 0;
}

#endif  // BAZEL_TEMPLATE_CLIENT_HTTP_FILE_CLIENT_H
