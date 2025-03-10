/*
 * Copyright 2017 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/async_grpc/server.h"

#include "glog/logging.h"
#if BUILD_TRACING
#include "opencensus/exporters/trace/stackdriver/stackdriver_exporter.h"
#include "opencensus/trace/trace_config.h"
#endif

namespace async_grpc {
namespace {

const common::Duration kPopEventTimeout = common::FromMilliseconds(100);
constexpr unsigned int kDefaultTracingMaxAttributes = 128;
constexpr unsigned int kDefaultTracingMaxAnnotations = 128;
constexpr unsigned int kDefaultTracingMaxMessageEvents = 128;
constexpr unsigned int kDefaultTracingMaxLinks = 128;

}  // namespace

void Server::Builder::SetNumGrpcThreads(const size_t num_grpc_threads) {
  options_.num_grpc_threads = num_grpc_threads;
}

void Server::Builder::SetNumEventThreads(const std::size_t num_event_threads) {
  options_.num_event_threads = num_event_threads;
}

void Server::Builder::SetServerAddress(const std::string& server_address) {
  options_.server_address = server_address;
}

void Server::Builder::SetMaxReceiveMessageSize(int max_receive_message_size) {
  CHECK_GT(max_receive_message_size, 0)
      << "max_receive_message_size must be larger than 0.";
  options_.max_receive_message_size = max_receive_message_size;
}

void Server::Builder::SetMaxSendMessageSize(int max_send_message_size) {
  CHECK_GT(max_send_message_size, 0)
      << "max_send_message_size must be larger than 0.";
  options_.max_send_message_size = max_send_message_size;
}

void Server::Builder::EnableTracing() {
#if BUILD_TRACING
  options_.enable_tracing = true;
#else
  LOG(FATAL) << "Enable tracing support by compiling with -DBUILD_TRACING=1.";
#endif
}

void Server::Builder::DisableTracing() { options_.enable_tracing = false; }

void Server::Builder::SetTracingSamplerProbability(
    double tracing_sampler_probability) {
  options_.tracing_sampler_probability = tracing_sampler_probability;
}

void Server::Builder::SetTracingTaskName(const std::string& tracing_task_name) {
  options_.tracing_task_name = tracing_task_name;
}

void Server::Builder::SetTracingGcpProjectId(
    const std::string& tracing_gcp_project_id) {
  options_.tracing_gcp_project_id = tracing_gcp_project_id;
}

std::tuple<std::string, std::string> Server::Builder::ParseMethodFullName(
    const std::string& method_full_name) {
  CHECK(method_full_name.at(0) == '/') << "Invalid method name.";
  std::stringstream stream(method_full_name.substr(1));
  std::string service_full_name;
  std::getline(stream, service_full_name, '/');
  std::string method_name;
  std::getline(stream, method_name, '/');
  CHECK(!service_full_name.empty() && !method_name.empty());
  return std::make_tuple(service_full_name, method_name);
}

std::unique_ptr<Server> Server::Builder::Build() {
  std::unique_ptr<Server> server(new Server(options_));
  for (const auto& service_handlers : rpc_handlers_) {
    server->AddService(service_handlers.first, service_handlers.second);
  }
  return server;
}

Server::Server(const Options& options) : options_(options) {
  server_builder_.AddListeningPort(options_.server_address,
                                   ::grpc::InsecureServerCredentials());

  // Set max message sizes.
  server_builder_.SetMaxReceiveMessageSize(options.max_receive_message_size);
  server_builder_.SetMaxSendMessageSize(options.max_send_message_size);

  // Set up event queue threads.
  event_queue_threads_ =
      std::vector<EventQueueThread>(options_.num_event_threads);

  // Set up completion queues threads.
  for (size_t i = 0; i < options_.num_grpc_threads; ++i) {
    completion_queue_threads_.emplace_back(
        server_builder_.AddCompletionQueue());
  }
}

void Server::AddService(
    const std::string& service_name,
    const std::map<std::string, RpcHandlerInfo>& rpc_handler_infos) {
  // Instantiate and register service.
  const auto result = services_.emplace(
      std::piecewise_construct, std::make_tuple(service_name),
      std::make_tuple(service_name, rpc_handler_infos,
                      [this]() { return SelectNextEventQueueRoundRobin(); }));
  CHECK(result.second) << "A service named " << service_name
                       << " already exists.";
  server_builder_.RegisterService(&result.first->second);
}

void Server::RunCompletionQueue(
    ::grpc::ServerCompletionQueue* completion_queue) {
  bool ok;
  void* tag;
  while (completion_queue->Next(&tag, &ok)) {
    auto* rpc_event = static_cast<Rpc::CompletionQueueRpcEvent*>(tag);
    rpc_event->ok = ok;
    rpc_event->PushToEventQueue();
  }
}

EventQueue* Server::SelectNextEventQueueRoundRobin() {
  common::MutexLocker locker(&current_event_queue_id_lock_);
  current_event_queue_id_ =
      (current_event_queue_id_ + 1) % options_.num_event_threads;
  return event_queue_threads_.at(current_event_queue_id_).event_queue();
}

void Server::RunEventQueue(EventQueue* event_queue) {
  while (!shutting_down_) {
    Rpc::UniqueEventPtr rpc_event =
        event_queue->PopWithTimeout(kPopEventTimeout);
    if (rpc_event) {
      rpc_event->Handle();
    }
  }

  // Finish processing the rest of the items.
  while (Rpc::UniqueEventPtr rpc_event =
             event_queue->PopWithTimeout(kPopEventTimeout)) {
    rpc_event->Handle();
  }
}

void Server::Start() {
#if BUILD_TRACING
  if (options_.enable_tracing) {
    opencensus::exporters::trace::StackdriverExporter::Register(
        options_.tracing_gcp_project_id);
    opencensus::trace::TraceConfig::SetCurrentTraceParams(
        {kDefaultTracingMaxAttributes, kDefaultTracingMaxAnnotations,
         kDefaultTracingMaxMessageEvents, kDefaultTracingMaxLinks,
         opencensus::trace::ProbabilitySampler(
             options_.tracing_sampler_probability)});
  }
#endif

  // Start the gRPC server process.
  server_ = server_builder_.BuildAndStart();

  // Start serving all services on all completion queues.
  for (auto& service : services_) {
    service.second.StartServing(completion_queue_threads_,
                                execution_context_.get());
  }

  // Start threads to process all event queues.
  for (auto& event_queue_thread : event_queue_threads_) {
    event_queue_thread.Start(
        [this](EventQueue* event_queue) { RunEventQueue(event_queue); });
  }

  // Start threads to process all completion queues.
  for (auto& completion_queue_threads : completion_queue_threads_) {
    completion_queue_threads.Start(
        [this](::grpc::ServerCompletionQueue* completion_queue) {
          RunCompletionQueue(completion_queue);
        });
  }
}

void Server::WaitForShutdown() {
  if (!server_) {
    return;
  }

  server_->Wait();
}

void Server::Shutdown() {
  LOG(INFO) << "Shutting down server.";
  shutting_down_ = true;

  // Tell the services to stop serving RPCs.
  for (auto& service : services_) {
    service.second.StopServing();
  }

  // Shut down the gRPC server waiting for RPCs to finish until the hard
  // deadline; then force a shutdown.
  server_->Shutdown();

  // Shut down the server completion queues and wait for the processing threads
  // to join.
  for (auto& completion_queue_threads : completion_queue_threads_) {
    completion_queue_threads.Shutdown();
  }

  for (auto& event_queue_thread : event_queue_threads_) {
    event_queue_thread.Shutdown();
  }

  LOG(INFO) << "Shutdown complete.";
}

void Server::SetExecutionContext(
    std::shared_ptr<ExecutionContext> execution_context) {
  // After the server has been started the 'ExecutionHandle' cannot be changed
  // anymore.
  CHECK(!server_);
  execution_context_ = std::move(execution_context);
}

}  // namespace async_grpc
