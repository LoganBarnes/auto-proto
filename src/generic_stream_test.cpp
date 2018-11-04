// current project
#include "util/util.h"
#include "util/message_util.h"

// external
#include <proto_reflection_descriptor_database.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/completion_queue.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

// standard
#include <string>
#include <memory>
#include <thread>

namespace gp = google::protobuf;

namespace {

std::string method_call_string(const gp::MethodDescriptor* method) {
    // needs to be "/package.Service/MethodName"
    std::string method_name = "/";

    const std::string& full_name = method->full_name();
    const std::string& name = method->name();

    method_name += full_name.substr(0, full_name.rfind(name) - 1); // package.Service
    method_name += "/" + name; // MethodName

    return method_name;
}

} // namespace

int main(int argc, const char* argv[]) {
    std::string server_address("0.0.0.0:50050");

    if (argc > 1) {
        server_address = argv[1];
    }

    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    grpc::GenericStub generic_stub(channel);

    int max_connection_attempts = 5;
    int current_connection_attempt = 1;
    do {
        int unused;
        grpc::CompletionQueue queue;

        bool ok;
        void* ptr = &unused;
        auto channel_state = channel->GetState(true);

        if (channel_state != GRPC_CHANNEL_READY) {
            // never wait more than 15 seconds (super max upper bound we shouldn't actually reach)
            auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(15);
            channel->NotifyOnStateChange(channel_state, deadline, &queue, &unused);

            std::cout << "Attempting to connect to '" << server_address << "' (attempt " << current_connection_attempt
                      << " of " << max_connection_attempts << ")." << std::endl;

            queue.Next(&ptr, &ok);
        }
    } while (current_connection_attempt++ < max_connection_attempts);

    if (channel->GetState(true) != GRPC_CHANNEL_READY) {
        std::cout << "Failed to connect to a server. Exiting client." << std::endl;
        return 0;
    }

    grpc::ProtoReflectionDescriptorDatabase reflection_database(channel);
    google::protobuf::DescriptorPool desc_pool(&reflection_database);

    std::vector<std::string> available_services;

    reflection_database.GetServices(&available_services);
    util::remove_by_value(&available_services, std::string("grpc.reflection.v1alpha.ServerReflection"));

    for (const std::string& service_name : available_services) {
        const gp::ServiceDescriptor* service_desc = desc_pool.FindServiceByName(service_name);
        assert(service_desc);

        std::cout << "Service: " << service_name << std::endl;

        int method_count = service_desc->method_count();

        for (int i = 0; i < method_count; ++i) {
            const gp::MethodDescriptor* method = service_desc->method(i);
            std::cout << "\t" << method_call_string(method) << ": " << method->DebugString();
        }
    }

    google::protobuf::DynamicMessageFactory message_factory;

    {
        const gp::ServiceDescriptor* service_desc = desc_pool.FindServiceByName("proj.proto.Server");
        const gp::MethodDescriptor* method = service_desc->method(1);
        std::string method_name = method_call_string(method);

        // serialize into a byte buffer so we can make a generic rpc call with the bytes
        std::shared_ptr<gp::Message> input_msg = util::clone_msg(*message_factory.GetPrototype(method->input_type()));
        std::unique_ptr<grpc::ByteBuffer> input_bytes = util::serialize_to_byte_buffer(*input_msg);

        // setup the rcp call (doesn't call yet)
        grpc::CompletionQueue cq;
        grpc::ClientContext context;
        std::unique_ptr<grpc::GenericClientAsyncReaderWriter> call;
        call = generic_stub.PrepareCall(&context, method_name, &cq);

        if (not call) {
            std::cerr << "Failed to prepare call" << std::endl;
            return 0;
        }

        auto tag = reinterpret_cast<void*>(1);

        call->StartCall(tag);
        void* got_tag;
        bool ok;
        cq.Next(&got_tag, &ok);
        assert(ok);

        call->Write(*input_bytes, tag);
        cq.Next(&got_tag, &ok);
        assert(ok);

        call->WritesDone(tag);
        cq.Next(&got_tag, &ok);
        assert(ok);

        std::thread run_thread([&] {
            std::shared_ptr<gp::Message> output_msg
                = util::clone_msg(*message_factory.GetPrototype(method->output_type()));
            grpc::ByteBuffer output_bytes;

            call->Read(&output_bytes, tag);

            std::cout << "Waiting for read" << std::endl;
            while (cq.Next(&got_tag, &ok) and ok) {

                util::deserialize_from_byte_buffer(&output_bytes, output_msg.get());
                std::cout << "Output:\n\t" << output_msg->DebugString();

                call->Read(&output_bytes, tag);
                std::cout << "Waiting for read" << std::endl;
            }

            std::cout << "Exiting read loop" << std::endl;

            grpc::Status status;
            call->Finish(&status, tag);
            cq.Next(&got_tag, &ok);
            assert(ok);

            std::cout << "Finish status: " << (status.ok() ? "OK" : "") << status.error_message() << std::endl;
        });

        std::cout << "Press enter to exit." << std::endl;
        std::cin.ignore();

        context.TryCancel();
        run_thread.join();
    }

    return 0;
}
