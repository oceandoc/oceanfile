#include <condition_variable>
#include <mutex>

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/support/client_callback.h"
#include "src/proto/service.grpc.pb.h"
#include "src/proto/service.pb.h"

using grpc::ClientContext;
using grpc::Status;

namespace oceandoc {
namespace client {

class MathClient
    : public grpc::ClientBidiReactor<proto::FileReq, proto::FileRes> {
 public:
  explicit MathClient(proto::OceanFile::Stub* stub) {
    stub->async()->PutFile(&context_, this);
    Write();
    StartRead(&res_);
    StartCall();
  }

  void OnWriteDone(bool ok) override {
    if (ok) {
      Write();
    }
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      StartRead(&res_);
    }
  }

  void OnDone(const grpc::Status& s) override {
    std::unique_lock<std::mutex> l(mu_);
    status_ = s;
    done_ = true;
    cv_.notify_one();
  }

  grpc::Status Await() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this] { return done_; });
    return std::move(status_);
  }

 private:
  void Write() {
    std::unique_lock lock(mu_);
    volatile static int input = 5;
    StartWrite(&req_);
    LOG(INFO) << "Write done: " << input;
    if (input == 7) {
      StartWritesDone();
    }
    ++input;
  }

 private:
  ClientContext context_;
  proto::FileReq req_;
  proto::FileRes res_;
  std::mutex mu_;
  std::condition_variable cv_;
  Status status_;
  bool done_ = false;
};

}  // namespace client
}  // namespace oceandoc

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);
  LOG(INFO) << "Program initializing ...";
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  auto channel = grpc::CreateChannel("localhost:10001",
                                     grpc::InsecureChannelCredentials());
  std::unique_ptr<oceandoc::proto::OceanFile::Stub> stub(
      oceandoc::proto::OceanFile::NewStub(channel));
  oceandoc::client::MathClient match_client(stub.get());
  Status status = match_client.Await();
  if (!status.ok()) {
    LOG(ERROR) << "Math rpc failed.";
  }
  return 0;
}
