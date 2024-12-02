/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_CONFIG_MANAGER_H
#define BAZEL_TEMPLATE_UTIL_CONFIG_MANAGER_H

#include <memory>
#include <string>

#include "folly/Singleton.h"
#include "glog/logging.h"
#include "src/proto/config.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace util {

class ConfigManager {
 private:
  friend class folly::Singleton<ConfigManager>;
  ConfigManager() = default;

 public:
  static std::shared_ptr<ConfigManager> Instance();

  bool Init(const std::string& base_config_path) {
    std::string content;
    auto ret = Util::LoadSmallFile(base_config_path, &content);
    if (ret && !Util::JsonToMessage(content, &base_config_)) {
      LOG(ERROR) << "parse base config error, path: " << base_config_path
                 << ", content: " << content;
      return false;
    }
    LOG(INFO) << "base config: " << ToString();
    return true;
  }

  std::string ServerAddr() { return base_config_.server_addr(); }
  uint32_t GrpcServerPort() { return base_config_.grpc_server_port(); }
  uint32_t UdpServerPort() { return base_config_.grpc_server_port(); }
  uint32_t HttpServerPort() { return base_config_.http_server_port(); }
  uint32_t MetricRatio() { return base_config_.metric_ratio(); }
  uint32_t MetricIntervalSec() { return base_config_.metric_interval_sec(); }
  uint32_t DiscardRatio() { return base_config_.discard_ratio(); }
  uint32_t GrpcThreads() { return base_config_.grpc_threads(); }
  uint32_t EventThreads() { return base_config_.event_threads(); }
  uint32_t ReceiveQueueTimeout() { return 5 * 60 * 1000; }
  std::string SslCa() { return base_config_.server_ssl_ca(); }
  std::string SslCert() { return base_config_.server_ssl_cert(); }
  std::string SslKey() { return base_config_.server_ssl_key(); }
  bool UseHttps() { return base_config_.use_https(); }

  std::string ToString() {
    std::string json;
    Util::MessageToJson(base_config_, &json);
    return json;
  }

 private:
  oceandoc::proto::BaseConfig base_config_;
};

}  // namespace util
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_UTIL_CONFIG_MANAGER_H
