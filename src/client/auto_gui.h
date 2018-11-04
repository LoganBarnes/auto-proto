#pragma once

#include "client/proto_gui.h"

#include <proto_reflection_descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <grpcpp/generic/generic_stub.h>

namespace proj {

class MessageTree;

class AutoGui {
public:
    explicit AutoGui(const std::shared_ptr<grpc::Channel>& channel);
    ~AutoGui();

    void configure_gui(const GuiOptions& options);

private:
    grpc::GenericStub generic_stub_;

    grpc::ProtoReflectionDescriptorDatabase reflection_database_;
    google::protobuf::DescriptorPool desc_pool_;

    std::vector<std::string> available_services_;

    std::unique_ptr<MessageTree> messages_;
};

} // namespace proj
