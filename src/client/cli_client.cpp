#include "client/cli_client.h"

#include <grpcpp/create_channel.h>

namespace gp = google::protobuf;

namespace proj {

CliClient::CliClient(std::string server_address)
    : server_address_(std::move(server_address))
    , channel_(grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials()))
    , stub_(proj::proto::Server::NewStub(channel_)) {

    receive_thread_ = std::thread([&] {
        google::protobuf::Empty unused;
        std::unique_ptr<grpc::ClientReader<proj::proto::Sink2>> stream;

        stream = stub_->stream_state2(&context_, unused);

        proj::proto::Sink2 state;
        while (stream->Read(&state)) {
            std::cout << "*******************************************\n";
            std::cout << "RECEIVED STATE: \n";
            std::cout << state.DebugString();
            std::cout << "*******************************************\n";
            std::cout << std::flush;
        }
        stream->Finish();
    });
}

CliClient::~CliClient() {
    context_.TryCancel();
    receive_thread_.join();
}

void CliClient::dispatch_action(const proj::proto::Actions& action) {

    proj::proto::Response response;

    grpc::ClientContext context;
    grpc::Status status = stub_->dispatch_action(&context, action, &response);

    if (status.ok()) {
        std::cout << "Status: OK" << std::endl;
        if (response.error_msg().empty()) {
            std::cout << "Response Error: NONE" << std::endl;
        } else {
            std::cout << "Response Error: " << response.error_msg() << std::endl;
        }
    } else {
        std::cerr << "Status: " << status.error_message() << std::endl;
    }
}

void CliClient::run_loop() {

    std::string input;
    proj::proto::Actions action;

    while (true) {

        const gp::Descriptor* desc = action.GetDescriptor();

        int field_count = desc->field_count();

        const gp::FieldDescriptor* field = nullptr;

        while (not field) {
            std::cout << "Action type: " << std::endl;

            for (int i = 0; i < field_count; ++i) {
                const gp::FieldDescriptor* f = desc->field(i);
                std::cout << "\t" << f->name() << std::endl;
            }
            std::cout << "Enter valid action type: ";
            std::cin >> input;

            if (not std::cin) {
                break;
            }

            field = desc->FindFieldByName(input);
        }

        if (not std::cin) {
            break;
        }

        std::cout << "Enter value: ";
        std::cin >> input;

        if (!std::cin) {
            break;
        }

        auto msg = action.GetReflection()->MutableMessage(&action, field);

        msg->GetReflection()->SetString(msg, msg->GetDescriptor()->FindFieldByName("state"), input);

        dispatch_action(action);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << std::endl;
}

} // namespace proj