#if 1
// project
#include <client/cli_client.h>
#include <client/gui_client.h>
#include <server/server.h>

int main(int argc, const char* argv[]) {
    std::string server_address("0.0.0.0:50050");

    if (argc > 1) {
        server_address = argv[1];
    }

    svr::Server server(server_address);

    if (argc > 2 and argv[2] == std::string("--cli")) {
        proj::CliClient client(server_address);
        client.run_loop();
    } else {
        proj::GuiClient client(server_address);
        client.run_loop();
    }

    return 0;
}
#else

#include <proj/server.grpc.pb.h>

#include <proto_reflection_descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <util/message_util.h>

int main() {

    //    std::string server_address = "localhost:50050";
    //    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    //    std::unique_ptr<proj::proto::Server::Stub> stub = proj::proto::Server::NewStub(channel);
    //
    //    grpc::ProtoReflectionDescriptorDatabase reflection_database(channel);
    //    google::protobuf::DescriptorPool desc_pool;

    google::protobuf::DynamicMessageFactory message_factory;

    proj::proto::SemiShared semi_shared;

    const google::protobuf::Descriptor* desc = semi_shared.GetDescriptor();
    const google::protobuf::Message* message = message_factory.GetPrototype(desc);

    std::unique_ptr<google::protobuf::Message> msg_copy1 = util::clone_msg(message);
    std::unique_ptr<google::protobuf::Message> msg_copy2 = util::clone_msg(message);

    std::cout << "Descriptor:     " << desc << std::endl;
    std::cout << "Message:        " << message << std::endl;
    std::cout << "Message Copy 1: " << msg_copy1.get() << std::endl;
    std::cout << "Message Copy 2: " << msg_copy2.get() << std::endl;

    return 0;
}
#endif
