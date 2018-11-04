#include "server/server_tree.h"
#include "util/message_util.h"
#include "util/util.h"
#include "server_tree.h"

#include <proj/annotations.pb.h>

#include <unordered_map>
#include <sstream>
#include <thread>

#define ADD_SLEEPS

#ifdef ADD_SLEEPS
#define MAYBE_SLEEP_MS() std::this_thread::sleep_for(std::chrono::milliseconds(1000))
#else
#define MAYBE_SLEEP_MS()                                                                                               \
    {}
#endif

namespace gp = google::protobuf;

namespace svr {

ServerTree::Computer::~Computer() = default;
ServerTree::Sink::~Sink() = default;

ServerTree::ServerNode::ServerNode(std::unique_ptr<google::protobuf::Message> msg) : message(std::move(msg)) {
    debug_name = message->GetDescriptor()->name();
}

bool ServerTree::update_source(const google::protobuf::Message& message) {
    NodeKey key = get_key(message);

    auto iter = sources_.find(key);
    if (iter == sources_.end()) {
        return false;
    }

    // TODO: Error check send back errors before updating server in separate thread
    // Copy source data into node
    nodes_.at(key)->message->CopyFrom(message);

    {
        std::unordered_set<NodeKey> updated_sinks = invalidate_node(key, -1);

        for (NodeKey sink_key : updated_sinks) {
            auto& sink = sinks_.at(sink_key);
            sink->send_data();
        }
    }

    MAYBE_SLEEP_MS();

    {
        std::unordered_set<NodeKey> updated_sinks = update_node(key, -1);

        for (NodeKey sink_key : updated_sinks) {
            auto& sink = sinks_.at(sink_key);
            sink->send_data();
        }
    }

    {
        std::ofstream graphvis_file("nodes.dot.ps");
        graphvis_file << graphvis_string();
    }
    return true;
}

std::string ServerTree::graphvis_string() const {
    std::stringstream ss;
    ss << "digraph {\n";

    for (const auto& node_pair : nodes_) {
        const ServerNode& node = *node_pair.second;
        const std::string& node_name = node.debug_name;

        std::string color;
        std::string shape;

        if (node.outputs.empty()) {
            color = "azure";
            shape = "trapezium";

        } else if (node.inputs.empty()) {
            color = "darkolivegreen";
            shape = "invtrapezium";

        } else /*if (node.type == proj::proto::Node::SYNC)*/ {
            color = "burlywood";
            shape = "box";

            //        } else {
            //            assert(node_type == proj::proto::Node::ASYNC);
            //            color = "lightsalmon";
            //            shape = "octagon";
        }

        color += (node.valid ? "1" : "4");

        ss << "\t" << node_name << " [shape=" << shape << ", style=filled, fillcolor=" << color;
        ss << ", label=<" << node_name << "<font point-size=\"10\">";

        int num_fields = 0;

        util::iterate_msg_fields(node.message.get(), [&](const gp::FieldDescriptor* field, int /*index*/) {
            if (field->cpp_type() != gp::FieldDescriptor::CPPTYPE_MESSAGE) {
                ss << "<br/>" << field->name() << ": ";
                util::print_field(ss, *node.message, field, node.message->GetReflection());
                ++num_fields;

            } else {
                // Messages
                // TODO: print names of non-input messages
            }
        });
        if (num_fields == 0) {
            ss << "<br/>(no data)";
        }
        ss << "</font>>]\n";

        for (const auto& output_pair : node.outputs) {
            const ServerNode& output = *nodes_.at(output_pair.first);
            const std::string& output_name = output.debug_name;
            ss << "\t" << node_name << " -> {" << output_name << "} "
               << "[color=blue4, label=output]"
               << "\n";
        }

        for (const auto& input_pair : node.inputs) {
            const ServerNode& input = *nodes_.at(input_pair.second);
            const std::string& input_name = input.debug_name;
            ss << "\t" << input_name << " -> {" << node_name << "} "
               << "[color=green4, label=input]"
               << "\n";
        }
    }

    ss << "}\n";

    return ss.str();
}

ServerTree::NodeKey ServerTree::get_key(const google::protobuf::Descriptor* desc) {
    return desc;
}

ServerTree::NodeKey ServerTree::get_key(const google::protobuf::Message& message) {
    return get_key(message.GetDescriptor());
}

ServerTree::NodeKey ServerTree::get_key(google::protobuf::Message* message) {
    return get_key(*message);
}

ServerTree::NodeKey ServerTree::build_node(google::protobuf::Message* message) {
    NodeKey key = get_key(message);
    util::MsgPkg msg_pkg(message);

    auto iter = nodes_.find(key);
    if (iter == nodes_.end()) {

        proj::proto::Node node_type = msg_pkg.desc->options().GetExtension(proj::proto::node);

        if (node_type == proj::proto::Node::NONE) {
            return nullptr;
        }

        auto node = util::make_unique<ServerNode>(util::clone_msg(*message));
        std::tie(iter, std::ignore) = nodes_.emplace(key, std::move(node));
    } else {
        return key;
    }

    auto& node = *iter->second;
    msg_pkg = util::MsgPkg(node.message.get());

    util::iterate_msg_fields(*node.message, [&](const gp::FieldDescriptor* field, int field_index) {
        if (field->is_repeated()) {
            return; // Temporarily skipping repeated fields
        }

        msg_pkg.set_field_index(field_index);

        if (field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {

            if (field->containing_oneof()) {
                util::MsgPkg child_pkg(msg_pkg.refl->MutableMessage(msg_pkg.msg, field));
                if (child_pkg.desc->options().GetExtension(proj::proto::node) != proj::proto::Node::NONE) {
                    throw std::runtime_error(child_pkg.desc->name() + " node cannot be part of a 'oneof'");
                }

            } else if (auto input = build_node(msg_pkg.refl->MutableMessage(msg_pkg.msg, field))) {
                node.inputs.emplace(field_index, input);
                auto& input_node = *nodes_.at(input);
                input_node.outputs.emplace(key, field_index);
            }

            msg_pkg.refl->ClearField(msg_pkg.msg, msg_pkg.field);
        }
    });

    if (node.inputs.empty()) {
        sources_.emplace(key);
        std::cout << "SOURCE: " << msg_pkg.desc->name() << std::endl;
    } else {
        std::cout << node.inputs.size() << " input(s) for " << msg_pkg.desc->name() << std::endl;
    }

    return key;
}

std::unordered_set<ServerTree::NodeKey> ServerTree::invalidate_node(NodeKey key, int input_index) {

    ServerNode& node = *nodes_.at(key);
    util::MsgPkg msg_pkg(node.message.get());

    if (input_index > -1) {

        // Clear all non-input fields and copy parent input
        util::iterate_msg_fields(msg_pkg.msg, [&](const gp::FieldDescriptor* field, int field_index) {
            auto iter = node.inputs.find(field_index);
            if (iter == node.inputs.end()) {
                msg_pkg.refl->ClearField(msg_pkg.msg, field);

            } else if (input_index == field_index) {
                const ServerNode& input_node = *nodes_.at(iter->second);
                msg_pkg.refl->MutableMessage(msg_pkg.msg, field)->CopyFrom(*input_node.message);
            }
        });

        // Mark as invalid
        node.valid = false;
    }

    if (not node.outputs.empty()) {

        std::unordered_set<NodeKey> updated_sinks;

        // Invalidate all children nodes
        for (const auto& output_key_pair : node.outputs) {
            auto sinks = invalidate_node(output_key_pair.first, output_key_pair.second);
            updated_sinks.insert(sinks.begin(), sinks.end());
        }
        return updated_sinks;
    }

    // Must be a sink
    assert(sinks_.find(key) != sinks_.end());
    auto& sink = sinks_.at(key);
    sink->get_data()->CopyFrom(*node.message);
    return {key};
}

std::unordered_set<ServerTree::NodeKey> ServerTree::update_node(NodeKey key, int input_index) {

    ServerNode& node = *nodes_.at(key);
    util::MsgPkg msg_pkg(node.message.get());

    bool valid_before = node.valid;

    // Check if all inputs are valid
    node.valid = true;
    for (const auto& input_key_pair : node.inputs) {
        const ServerNode& input_node = *nodes_.at(input_key_pair.second);
        node.valid &= input_node.valid;

        if (input_index == input_key_pair.first) {
            msg_pkg.set_field_index(input_key_pair.first);
            msg_pkg.refl->MutableMessage(msg_pkg.msg, msg_pkg.field)->CopyFrom(*input_node.message);
        }
    }

    if (node.valid) {
        // Run the registered compute function if it exists
        auto iter = compute_functions_.find(key);
        if (iter != compute_functions_.end()) {
            // TODO: make COMPUTE_FUNC macro
            iter->second->compute(msg_pkg.msg);
        }
    }

    if (valid_before != node.valid) {
        {
            std::ofstream graphvis_file("nodes.dot.ps");
            graphvis_file << graphvis_string();
        }
        MAYBE_SLEEP_MS();
    }

    if (not node.outputs.empty()) {
        std::unordered_set<NodeKey> updated_sinks;

        // Update children
        for (const auto& output_key_pair : node.outputs) {
            auto sinks = update_node(output_key_pair.first, output_key_pair.second);
            updated_sinks.insert(sinks.begin(), sinks.end());
        }
        return updated_sinks;
    }

    assert(sinks_.find(key) != sinks_.end());
    auto& sink = sinks_.at(key);
    sink->get_data()->CopyFrom(*node.message);
    return {key};
}

std::unordered_set<gp::Message*> ServerTree::copy_node_to_messages(
    const ServerNode& node, int field_index, const std::unordered_set<gp::Message*>& parent_msgs) {
    std::unordered_set<gp::Message*> msgs_to_return;

    for (gp::Message* msg : parent_msgs) {
        util::MsgPkg data_pkg(msg);
        data_pkg.set_field_index(field_index);
        gp::Message* msg_to_update = data_pkg.refl->MutableMessage(data_pkg.msg, data_pkg.field);

#if 1
        msg_to_update->CopyFrom(*node.message);
#else
        util::iterate_msg_fields(msg_to_update, [&](const gp::FieldDescriptor*, int input_index) {
            if (node.inputs.find(input_index) == node.inputs.end()) {
                util::copy_field(msg_to_update, *node.message, input_index);
            }
        });
#endif

        msgs_to_return.emplace(msg_to_update);
    }
    return msgs_to_return;
}

} // namespace svr
