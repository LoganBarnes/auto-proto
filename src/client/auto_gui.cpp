#include "client/auto_gui.h"
#include "client/message_tree.h"
#include "auto_gui.h"
#include "util/generic_guard.h"
#include "util/message_util.h"
#include "util/util.h"

#include <grpcpp/channel.h>
#include <google/protobuf/descriptor.h>

#include <imgui.h>

namespace gp = google::protobuf;

namespace proj {

namespace {

enum class RpcMsgType { INPUT, OUTPUT };

std::string rpc_message_key(const gp::MethodDescriptor* method, RpcMsgType type) {
    return method->name() + '_' + (type == RpcMsgType::INPUT ? method->input_type() : method->output_type())->name();
}

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

AutoGui::AutoGui(const std::shared_ptr<grpc::Channel>& channel)
    : generic_stub_(channel)
    , reflection_database_(channel)
    , desc_pool_(&reflection_database_)
    , messages_(std::make_unique<MessageTree>()) {

    reflection_database_.GetServices(&available_services_);
    util::remove_by_value(&available_services_, std::string("grpc.reflection.v1alpha.ServerReflection"));

    for (const std::string& service_name : available_services_) {
        const gp::ServiceDescriptor* service_desc = desc_pool_.FindServiceByName(service_name);
        assert(service_desc);

        int method_count = service_desc->method_count();

        for (int i = 0; i < method_count; ++i) {
            const gp::MethodDescriptor* method = service_desc->method(i);

            messages_->add_message(rpc_message_key(method, RpcMsgType::INPUT), method->input_type());
            messages_->add_message(rpc_message_key(method, RpcMsgType::OUTPUT), method->output_type());
        }
    }
}

AutoGui::~AutoGui() = default;

void AutoGui::configure_gui(const GuiOptions& options) {

    for (const std::string& service_name : available_services_) {

        if (ImGui::CollapsingHeader(service_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto scoped_indent = util::make_guard([] { ImGui::Indent(); }, [] { ImGui::Unindent(); });

            const gp::ServiceDescriptor* service_desc = desc_pool_.FindServiceByName(service_name);

            if (not service_desc) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "service not found");
                continue;
            }

            int method_count = service_desc->method_count();

            for (int i = 0; i < method_count; ++i) {
                const gp::MethodDescriptor* method = service_desc->method(i);

                if (ImGui::CollapsingHeader(method->name().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

                    std::string input_name = "Input: " + method->input_type()->name();
                    if (method->client_streaming()) {
                        input_name += " (stream)";
                    }
                    if (ImGui::TreeNode(input_name.c_str())) {
                        auto scoped_tree_pop = util::make_guard([] {}, &ImGui::TreePop);

                        auto msg_key = rpc_message_key(method, RpcMsgType::INPUT);
                        bool something_changed = messages_->configure_gui(msg_key, options);

                        bool do_rpc_call;
                        if (options.auto_update) {
                            do_rpc_call = something_changed;
                        } else {
                            do_rpc_call = ImGui::Button("Send To Server");
                        }

                        if (do_rpc_call) {
                            std::string method_name = method_call_string(method);

                            // serialize into a byte buffer so we can make a generic rpc call with the bytes
                            const gp::Message* current_msg = messages_->get_message(msg_key);
                            std::unique_ptr<grpc::ByteBuffer> buffer = util::serialize_to_byte_buffer(*current_msg);

                            // setup the rcp call (doesn't call yet)
                            grpc::CompletionQueue cq;
                            grpc::ClientContext context;
                            auto call = generic_stub_.PrepareUnaryCall(&context, method_name, *buffer, &cq);

                            grpc::Status status;
                            grpc::ByteBuffer unused_output;

                            // make the call
                            call->StartCall();
                            call->Finish(&unused_output, &status, reinterpret_cast<void*>(1));

                            bool ok;
                            void* got_tag;
                            cq.Next(&got_tag, &ok);

                            std::cout << std::boolalpha << "ok: " << ok << std::endl;
                            std::cout << "status: " << (status.ok() ? "ok" : "not ok...") << status.error_message()
                                      << std::endl;
                        }
                    }

                    std::string output_name = "Output: " + method->output_type()->name();
                    if (method->server_streaming()) {
                        output_name += " (stream)";
                    }
                    if (ImGui::TreeNode(output_name.c_str())) {
                        auto scoped_tree_pop = util::make_guard([] {}, &ImGui::TreePop);
                        messages_->configure_gui(rpc_message_key(method, RpcMsgType::OUTPUT), options);
                    }
                }
            }
        }
    }
}

} // namespace proj
