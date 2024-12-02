/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/server/udp_server_impl.h"

namespace oceandoc {
namespace server {

void UdpServer::HandleMessage(std::size_t length) {
  std::string message(recv_buffer_.data(), length);
  LOG(INFO) << "Received " << length << " bytes from "
            << remote_endpoint_.address() << ":" << remote_endpoint_.port()
            << ": " << message;

  // Add your message handling logic here
  // For example, you might want to:
  // 1. Parse the message
  // 2. Process it
  // 3. Send a response if needed
}

}  // namespace server
}  // namespace oceandoc
