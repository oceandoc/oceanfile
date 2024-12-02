/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/server/udp_server_impl.h"

#include "boost/json.hpp"
#include "google/protobuf/util/json_util.h"
#include "src/proto/service.pb.h"

namespace oceandoc {
namespace server {

void UdpServer::HandleMessage(std::size_t length) {
  std::string json_str(recv_buffer_.data(), length);
  LOG(INFO) << "Received " << length << " bytes from "
            << remote_endpoint_.address() << ":" << remote_endpoint_.port()
            << ": " << json_str;

  try {
    // Parse JSON to determine message type
    boost::json::value json = boost::json::parse(json_str);
    if (!json.is_object()) {
      LOG(ERROR) << "Invalid JSON message: not an object";
      return;
    }

    auto obj = json.as_object();
    if (!obj.contains("type")) {
      LOG(ERROR) << "JSON message missing 'type' field";
      return;
    }

    std::string msg_type = boost::json::value_to<std::string>(obj.at("type"));
    if (!obj.contains("data")) {
      LOG(ERROR) << "JSON message missing 'data' field";
      return;
    }

    std::string json_data = boost::json::serialize(obj.at("data"));
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;

    if (msg_type == "UserReq") {
      proto::UserReq user_req;
      auto status = google::protobuf::util::JsonStringToMessage(json_data, &user_req, options);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to parse UserReq: " << status.message();
        return;
      }
      HandleUserRequest(user_req);
    } 
    else if (msg_type == "ServerReq") {
      proto::ServerReq server_req;
      auto status = google::protobuf::util::JsonStringToMessage(json_data, &server_req, options);
      if (!status.ok()) {
        LOG(ERROR) << "Failed to parse ServerReq: " << status.message();
        return;
      }
      HandleServerRequest(server_req);
    }
    else {
      LOG(ERROR) << "Unknown message type: " << msg_type;
    }
  }
  catch (const std::exception& e) {
    LOG(ERROR) << "Error processing message: " << e.what();
  }
}

void UdpServer::HandleUserRequest(const proto::UserReq& req) {
  LOG(INFO) << "Handling UserReq: op=" << proto::UserOp_Name(req.op())
            << ", user=" << req.user();
  
  proto::UserRes response;
  // TODO: Implement user request handling
  response.set_err_code(proto::ErrCode::OK);
  
  SendResponse("UserRes", response);
}

void UdpServer::HandleServerRequest(const proto::ServerReq& req) {
  LOG(INFO) << "Handling ServerReq: op=" << proto::ServerOp_Name(req.op())
            << ", path=" << req.path();
  
  proto::ServerRes response;
  // TODO: Implement server request handling
  response.set_err_code(proto::ErrCode::OK);
  
  SendResponse("ServerRes", response);
}

void UdpServer::SendResponse(const std::string& type, const google::protobuf::Message& message) {
  google::protobuf::util::JsonPrintOptions options;
  options.preserve_proto_field_names = true;
  std::string json_response;
  
  auto status = google::protobuf::util::MessageToJsonString(message, &json_response, options);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to convert response to JSON: " << status.message();
    return;
  }

  // Wrap in response object with type
  boost::json::object response_obj;
  response_obj["type"] = type;
  response_obj["data"] = boost::json::parse(json_response);
  
  std::string response_str = boost::json::serialize(response_obj);
  socket_.async_send_to(
      boost::asio::buffer(response_str), remote_endpoint_,
      [this](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
        if (ec) {
          LOG(ERROR) << "Failed to send response: " << ec.message();
        }
      });
}

}  // namespace server
}  // namespace oceandoc
