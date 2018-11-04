#pragma once

#include <proj/server.grpc.pb.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <memory>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <queue>
#include <util/blocking_deque.h>
#include "../../cmake-build-debug/protos/proto/proj/server.pb.h"

namespace svr {
class ServerTree;

class Compute;

template <typename T>
class StreamHandler;

class Server : private proj::proto::Server::Service {
public:
    explicit Server(std::string server_address);
    ~Server() override;

private:
    std::string server_address_;
    std::unique_ptr<grpc::Server> server_;
    grpc::reflection::ProtoServerReflectionPlugin plugin_;

    std::thread run_thread_;
    std::thread stream_thread_;
    std::shared_ptr<util::BlockingQueue<proj::proto::Sink2>> stream_queue_;

    std::unique_ptr<ServerTree> server_tree_;
    std::unique_ptr<StreamHandler<proj::proto::Sink2>> stream_handler_;

    std::unique_ptr<Compute> compute_test_;

    std::atomic_bool exit_stream_thread_;

    grpc::Status dispatch_action(grpc::ServerContext* context,
                                 const proj::proto::Actions* request,
                                 proj::proto::Response* response) override;

    //    grpc::Status
    //    stream_state2(grpc::ServerContext* context,
    //                  grpc::ServerReaderWriter<::proj::proto::Sink2, google::protobuf::Empty>* stream) override;

    grpc::Status stream_state2(grpc::ServerContext* context,
                               const google::protobuf::Empty* request,
                               grpc::ServerWriter<::proj::proto::Sink2>* writer) override;
};

} // namespace svr
