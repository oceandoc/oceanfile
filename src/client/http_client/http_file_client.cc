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
#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace client {

size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* stream) {
  common::UploadContext* context = static_cast<common::UploadContext*>(stream);
  if (!context->file.is_open()) {
    return CURL_READFUNC_ABORT;
  }
  context->file.read(static_cast<char*>(ptr), size * nmemb);
  context->remaining -= context->file.gcount();
  return context->file.gcount();
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                            std::string* response_string) {
  response_string->append(static_cast<char*>(contents), size * nmemb);
  return size * nmemb;
}

class FileClient {
 public:
  FileClient() : curl_(nullptr) { curl_ = curl_easy_init(); }

  ~FileClient() {
    if (curl_) {
      curl_easy_cleanup(curl_);
      curl_ = nullptr;
    }
  }

  bool Send(const std::string& url, const std::string& path,
            const std::string& repo_uuid) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file || !file.is_open()) {
      LOG(ERROR) << "Failed to open file: " << path;
      return false;
    }

    file.seekg(0);

    common::FileAttr attr;
    if (!util::Util::PrepareFile(path, common::Hash_BLAKE3,
                                 common::NET_BUFFER_SIZE_BYTES, &attr)) {
      LOG(ERROR) << "Prepare error: " << path;
      return false;
    }

    std::vector<char> buffer(common::NET_BUFFER_SIZE_BYTES);
    proto::FileReq req;
    req.mutable_content()->resize(common::NET_BUFFER_SIZE_BYTES);
    req.set_op(proto::FileOp::FilePut);
    req.set_dst(path);
    req.set_hash(attr.hash);
    req.set_size(attr.size);
    req.set_repo_uuid(repo_uuid);
    std::string serialized;

    int32_t partition_num = 0;
    while (file.read(buffer.data(), common::NET_BUFFER_SIZE_BYTES) ||
           file.gcount() > 0) {
      if (curl_) {
        req.mutable_content()->resize(file.gcount());
        req.set_partition_num(partition_num);
        std::copy(buffer.data(), buffer.data() + file.gcount(),
                  req.mutable_content()->begin());
        if (!util::Util::FileReqToJson(req, &serialized)) {
          // if (!req.SerializeToString(&serialized)) {
          LOG(ERROR) << "Req to json error: " << serialized;
        }

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        // curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
        // curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, serialized.data());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, serialized.size());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

        std::string response_string;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);

        LOG(INFO) << "Now send " << req.hash() << ", part: " << partition_num;
        CURLcode ret = curl_easy_perform(curl_);
        curl_slist_free_all(headers);
        if (ret != CURLE_OK) {
          LOG(ERROR) << "curl_easy_perform() failed: "
                     << curl_easy_strerror(ret);
          curl_easy_cleanup(curl_);
          curl_ = curl_easy_init();
          return false;
        }
        ++partition_num;
        proto::FileRes res;
        if (!util::Util::JsonToMessage(response_string, &res)) {
          // if(!res.ParseFromString(response_string)) {;
          // oceandoc::util::Util::PrintProtoMessage(res);
          LOG(ERROR) << "To Res message error";
        }
        LOG(INFO) << response_string;
        response_string.clear();
      }
    }

    return true;
  }

 private:
  CURL* curl_;
};

}  // namespace client
}  // namespace oceandoc

int main() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  oceandoc::client::FileClient file_client;
  file_client.Send(
      // "https://code.xiamu.com:10003/file",
      "http://code.xiamu.com:10003/file",
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus",
      "8636ac78-d409-4c27-8827-c6ddb1a3230c");

  curl_global_cleanup();
  return 0;
}

#endif  // BAZEL_TEMPLATE_CLIENT_HTTP_FILE_CLIENT_H
