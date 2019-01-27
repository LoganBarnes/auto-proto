#pragma once

#include "client/proto_gui.h"

#include <google/protobuf/dynamic_message.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace proj {

class MessageTree {
public:
    explicit MessageTree();
    ~MessageTree();

    bool add_message(const std::string& name, const google::protobuf::Descriptor* message);
    bool configure_gui(const std::string& name, const GuiOptions& options);
    const google::protobuf::Message* get_message(const std::string& name);

private:
    google::protobuf::DynamicMessageFactory message_factory_;

    struct MessageNode;
    std::unordered_map<std::string, std::unique_ptr<MessageNode>> roots_;

    std::unique_ptr<MessageNode> build_node(google::protobuf::Message* message);
    std::unique_ptr<MessageNode> build_node(const google::protobuf::Descriptor* message);

    bool configure_gui(const std::string& name, const MessageNode& node, const GuiOptions& options);
};

} // namespace proj
