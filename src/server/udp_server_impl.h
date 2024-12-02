/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H
#define BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H

#include <array>
#include <memory>
#include <string>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/udp.hpp"
#include "glog/logging.h"
#include "src/server/server_context.h"
#include "src/util/config_manager.h"

namespace oceandoc {
namespace server {

class UdpServer final {
 public:
  UdpServer(std::shared_ptr<ServerContext> server_context)
      : server_context_(server_context), io_context_(), socket_(io_context_) {
    std::string addr = util::ConfigManager::Instance()->ServerAddr();
    int32_t port = util::ConfigManager::Instance()->UdpServerPort();

    try {
      boost::asio::ip::udp::endpoint endpoint(
          boost::asio::ip::make_address(addr), port);
      socket_.open(endpoint.protocol());
      socket_.bind(endpoint);

      StartReceive();
    } catch (const boost::system::system_error& e) {
      LOG(ERROR) << "Failed to start UDP server: " << e.what();
      throw;
    }
  }

  void Start() {
    try {
      server_context_->MarkedUdpServerInitedDone();
      io_context_.run();
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error running UDP server: " << e.what();
      throw;
    }
  }

  void Shutdown() {
    try {
      socket_.close();
      io_context_.stop();
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error shutting down UDP server: " << e.what();
    }
  }

 private:
  static constexpr size_t MAX_BUFFER_SIZE = 65507;  // Max UDP packet size

  void StartReceive() {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_recvd) {
          if (!ec) {
            if (bytes_recvd == MAX_BUFFER_SIZE) {
              LOG(WARNING) << "Received UDP packet may have been truncated - "
                           << "size equals buffer size of " << MAX_BUFFER_SIZE;
            }
            HandleMessage(bytes_recvd);
            StartReceive();
          } else {
            LOG(ERROR) << "Receive error: " << ec.message();
          }
        });
  }

  void HandleMessage(std::size_t length);
  void SendResponse(const google::protobuf::Message& msg);

 private:
  std::shared_ptr<ServerContext> server_context_;
  boost::asio::io_context io_context_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remote_endpoint_;
  std::array<char, MAX_BUFFER_SIZE> recv_buffer_;
};

}  // namespace server
}  // namespace oceandoc

#endif  // BAZEL_TEMPLATE_SERVER_UDP_SERVER_IMPL_H
