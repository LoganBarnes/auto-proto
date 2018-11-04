#include "server/server.h"
#include "server/server_tree.h"
#include "server/stream_handler.h"
#include "server/compute_functions.h"
#include "server.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <sstream>
#include <fstream>
#include <util/message_util.h>

namespace gp = google::protobuf;

namespace svr {

Server::Server(std::string server_address)
    : server_address_(std::move(server_address))
    , stream_queue_(std::make_shared<util::BlockingQueue<proj::proto::Sink2>>())
    , server_tree_(util::make_unique<ServerTree>())
    , stream_handler_(util::make_unique<StreamHandler<proj::proto::Sink2>>())
    , compute_test_(util::make_unique<Compute>()) {

    server_tree_->add_output(stream_queue_);

    compute_test_->register_compute_functions(server_tree_.get());

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(this);

    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address_ << std::endl;

    {
        std::ofstream graphvis_file("server_state.dot.ps");
        proj::proto::Sink2 empty_state;
        graphvis_file << util::graphvis_string(empty_state);
    }

    exit_stream_thread_ = false;
    run_thread_ = std::thread([this] { server_->Wait(); });

    stream_thread_ = std::thread([this] {
        while (not exit_stream_thread_.load()) {
            auto updated_state = stream_queue_->pop_front();

            if (not exit_stream_thread_.load()) {
                {
                    std::ofstream graphvis_file("server_state.dot.ps");
                    graphvis_file << util::graphvis_string(updated_state);
                }
                {
                    std::ofstream graphvis_file("nodes.dot.ps");
                    graphvis_file << server_tree_->graphvis_string();
                }
            }

            stream_handler_->send_data(std::move(updated_state));
        }
    });
}

Server::~Server() {
    // Close the streaming client connections
    stream_handler_->attempt_shutdown();

    // The deadline forces calls to terminate even if they aren't completed.
    // This is necessary because the client is using a continuous streaming call.
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    server_->Shutdown(deadline);

    // Wait for the server to finish
    run_thread_.join();

    // Cause the `stream_thread_` loop to exit
    exit_stream_thread_.store(true);
    stream_queue_->push_back(proj::proto::Sink2());

    stream_thread_.join();
}

grpc::Status Server::dispatch_action(grpc::ServerContext* /*context*/,
                                     const proj::proto::Actions* request,
                                     proj::proto::Response* response) {

    bool action_received = false;

    util::iterate_msg_fields(*request, [&](const gp::FieldDescriptor* field, int /*index*/) {
        if (util::message_has_field(*request, field)) {
            action_received = true;
            const gp::Message& msg = request->GetReflection()->GetMessage(*request, field);

            std::cout << "Received " << msg.GetDescriptor()->name() << " Action" << std::endl;

            if (not server_tree_->update_source(msg)) {
                std::cerr << "Action does not correspond to a source" << std::endl;
            }
        }
    });

    if (not action_received) {
        response->set_error_msg("No action specified");
    }

    return grpc::Status::OK;
}

grpc::Status Server::stream_state2(grpc::ServerContext* context,
                                   const google::protobuf::Empty* /*request*/,
                                   grpc::ServerWriter<::proj::proto::Sink2>* writer) {
    std::cout << "Client connected" << std::endl;
    stream_handler_->handle_client(context, writer);
    std::cout << "Client disconnected" << std::endl;
    return grpc::Status::OK;
}

} // namespace svr
