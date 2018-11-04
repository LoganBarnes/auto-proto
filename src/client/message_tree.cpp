#include "client/message_tree.h"
#include "client/proto_gui.h"
#include "client/imgui_util.h"
#include "util/message_util.h"
#include "util/util.h"
#include "message_tree.h"

#include <regex>
#include <util/generic_guard.h>

namespace gp = google::protobuf;

namespace proj {

namespace {

std::string message_header(const std::string& name, const util::MsgPkg& msg_pkg, const GuiOptions& options) {
    std::string header = util::to_upper(name);
    header = std::regex_replace(header, std::regex("_"), std::string(" "));

    if (options.show_types) {
        header += " (" + msg_pkg.desc->name() + ')';
    }

    header += "###" + header; // hidden imgui label for this header
    return header;
}

} // namespace

struct MessageTree::MessageNode {
    std::unique_ptr<gp::Message> message;
    std::unordered_map<int, std::unique_ptr<MessageNode>> oneof_message_fields;
    std::unordered_map<int, std::unique_ptr<MessageNode>> message_fields;

    explicit MessageNode(std::unique_ptr<gp::Message> msg) : message(std::move(msg)) {}
};

MessageTree::MessageTree() = default;
MessageTree::~MessageTree() = default;

bool MessageTree::add_message(const std::string& name, const gp::Descriptor* message) {
    auto iter = roots_.find(name);

    if (iter != roots_.end()) {
        std::cerr << std::string(__FUNCTION__) + ": Key already exists." << std::endl;
        return false;
    }
    roots_.emplace(name, build_node(message));
    return true;
}

bool MessageTree::configure_gui(const std::string& name, const GuiOptions& options) {
    auto iter = roots_.find(name);

    if (iter == roots_.end()) {
        ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "'%s' not present in message tree", name.c_str());
        return false;
    }

    return configure_gui("", *iter->second, options);
}

const google::protobuf::Message* MessageTree::get_message(const std::string& name) {
    return roots_.at(name)->message.get();
}

std::unique_ptr<MessageTree::MessageNode> MessageTree::build_node(google::protobuf::Message* message) {
    return build_node(message->GetDescriptor());
}

std::unique_ptr<MessageTree::MessageNode> MessageTree::build_node(const google::protobuf::Descriptor* message) {
    auto node = std::make_unique<MessageTree::MessageNode>(util::clone_msg(*message_factory_.GetPrototype(message)));

    util::MsgPkg msg_pkg(node->message.get());

    util::iterate_msg_fields(*node->message, [&](const gp::FieldDescriptor* field, int field_index) {
        if (field->is_repeated()) {
            return; // Temporarily skipping repeated fields
        }

        msg_pkg.set_field_index(field_index);

        if (field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {

            if (field->containing_oneof()) {
                auto child_node = build_node(msg_pkg.refl->MutableMessage(msg_pkg.msg, field));
                node->oneof_message_fields.emplace(field->index_in_oneof(), std::move(child_node));
                msg_pkg.refl->ClearField(msg_pkg.msg, msg_pkg.field);

            } else {
                auto child_node = build_node(msg_pkg.refl->MutableMessage(msg_pkg.msg, field));
                node->message_fields.emplace(field_index, std::move(child_node));
            }
        }
    });

    return node;
}

bool MessageTree::configure_gui(const std::string& name, const MessageNode& node, const GuiOptions& options) {
    bool something_updated = false;

    util::MsgPkg msg_pkg(node.message.get());

    imgui::ScopedID scoped_node_id(msg_pkg.desc->full_name().c_str());

    std::string header_title = message_header(name, msg_pkg, options);

    auto header_flags = (options.headers_expanded_by_default ? ImGuiTreeNodeFlags_DefaultOpen : 0);

    if (name.empty() || ImGui::CollapsingHeader(header_title.c_str(), header_flags)) {
        imgui::ScopedIndent scoped_indent;

        util::iterate_msg_fields(msg_pkg.msg, [&](const gp::FieldDescriptor* field, int field_index) {
            imgui::ScopedID scoped_field_id(field_index);

            msg_pkg.set_field_index(field_index);

            const std::string& field_name = field->name();

            if (auto oneof_desc = field->containing_oneof()) {
                auto oneof_field_desc = msg_pkg.refl->GetOneofFieldDescriptor(*msg_pkg.msg, oneof_desc);

                if (oneof_field_desc and oneof_field_desc->index_in_oneof() != field->index_in_oneof()) {
                    // the oneof is set but the current field is not selected
                    return;
                }
                if (not oneof_field_desc and (not options.display_empty_fields or field->index_in_oneof() != 0)) {
                    // This oneof is not set and the option to display empty fields is false so we can exit.
                    return;
                }

                int field_count = oneof_desc->field_count();
                std::vector<std::string> types(static_cast<unsigned>(field_count));

                for (int fi = 0; fi < field_count; ++fi) {
                    std::string type = oneof_desc->field(fi)->name();
                    type = util::to_upper(type);
                    type = std::regex_replace(type, std::regex("_"), std::string(" "));
                    types[static_cast<unsigned>(fi)] = std::move(type);
                }

                int current_type;

                if (not oneof_field_desc) {
                    types.emplace_back("NOT SET");
                    current_type = field_count;
                } else {
                    current_type = oneof_field_desc->index_in_oneof();
                }

                bool oneof_type_updated = false;

                if (options.display_all_oneof_messsages) {
                    for (auto i = 0u; i < types.size(); ++i) {
                        oneof_type_updated |= ImGui::RadioButton(types[i].c_str(), &current_type, static_cast<int>(i));
                    }

                } else {
                    oneof_type_updated |= ImGui::Combo("##type",
                                                       &current_type,
                                                       [](void* data, int current_index, const char** out_str) {
                                                           auto strings = static_cast<std::string*>(data);
                                                           *out_str = strings[current_index].c_str();
                                                           return true;
                                                       },
                                                       types.data(),
                                                       static_cast<int>(types.size()));
                }

                if (oneof_type_updated) {

                    // Type has changed
                    auto new_field = msg_pkg.desc->FindFieldByName(oneof_desc->field(current_type)->name());

                    util::MsgPkg new_pkg(msg_pkg.msg);
                    new_pkg.set_field_index(new_field->index());

                    ProtoGui::activate_field(new_pkg);

                    something_updated = true;

                } else if (current_type == field->index_in_oneof()) {

                    if (field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {
                        const auto& child_node = node.oneof_message_fields.at(field->index_in_oneof());
                        something_updated |= configure_gui(field_name, *child_node, options);
                        msg_pkg.refl->MutableMessage(msg_pkg.msg, field)->CopyFrom(*child_node->message);

                    } else {
                        something_updated |= ProtoGui::configure_primitive_field(field_name, msg_pkg, options);
                    }
                }

            } else if (field->is_map()) {

            } else if (field->is_repeated()) {

            } else if (field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {
                const auto& child_node = node.message_fields.at(field->index());
                something_updated |= configure_gui(field_name, *child_node, options);
                msg_pkg.refl->MutableMessage(msg_pkg.msg, field)->CopyFrom(*child_node->message);

            } else {
                something_updated |= ProtoGui::configure_primitive_field(field_name, msg_pkg, options);
            }
        });
    }

    return something_updated;
}

} // namespace proj
