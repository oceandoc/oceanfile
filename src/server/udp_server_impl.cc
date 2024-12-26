/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/server/udp_server_impl.h"

#include "src/proto/service.pb.h"
#include "src/util/util.h"

namespace oceandoc {
namespace server {

void UdpServer::HandleMessage(std::size_t length) {
  std::string json_str(recv_buffer_.data(), length);
  LOG(INFO) << "Received " << length << " bytes from "
            << remote_endpoint_.address() << ":" << remote_endpoint_.port()
            << ": " << json_str;
  proto::ServerReq server_req;
  proto::ServerRes server_res;
  server_res.set_err_code(proto::ErrCode::Fail);
  if (!util::Util::JsonToMessage(json_str, &server_req)) {
    SendResponse(server_res);
  }
  if (server_req.op() == proto::ServerOp::ServerFindingServer) {
    server_res.set_err_code(proto::ErrCode::Success);
    server_res.set_handshake_msg("fstaion");
  }
  SendResponse(server_res);
}

void UdpServer::SendResponse(const google::protobuf::Message& message) {
  std::string json_str;
  util::Util::MessageToJson(message, &json_str);

  socket_.async_send_to(
      boost::asio::buffer(json_str), remote_endpoint_,
      [](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
        if (ec) {
          LOG(ERROR) << "Failed to send response: " << ec.message();
        }
      });
}

}  // namespace server
}  // namespace oceandoc
