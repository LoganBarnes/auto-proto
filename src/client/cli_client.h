#pragma once

#include <proj/server.grpc.pb.h>
#include <grpcpp/channel.h>
#include <thread>

namespace proj {

class CliClient {
public:
    explicit CliClient(std::string server_address);
    ~CliClient();

    void dispatch_action(const proj::proto::Actions& action);

    void run_loop();

private:
    std::string server_address_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<proj::proto::Server::Stub> stub_;

    std::thread receive_thread_;
    grpc::ClientContext context_;
};

} // namespace proj