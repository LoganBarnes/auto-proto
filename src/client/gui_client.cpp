#include "client/gui_client.h"
#include "client/auto_gui.h"
#include "client/theme.h"
#include "imgui_util.h"

#include <grpcpp/create_channel.h>
#include <fstream>

namespace gp = google::protobuf;

namespace proj {

namespace {} // namespace

GuiClient::GuiClient(std::string server_address)
    : server_address_(std::move(server_address))
    , gui_options_(std::make_unique<GuiOptions>())
    , theme_(std::make_unique<Theme>()) {

    reset_channel();

    ImGui::GetIO().Fonts->AddFontDefault();

    theme_->set_style();
    glClearColor(theme_->background.Value.x,
                 theme_->background.Value.y,
                 theme_->background.Value.z,
                 theme_->background.Value.w);

    //    gui_options_->auto_update = true;
    gui_options_->display_empty_fields = true;
    gui_options_->headers_expanded_by_default = true;
    gui_options_->display_all_oneof_messsages = true;
    gui_options_->strings_change_on_enter_only = false;
    gui_options_->label_width = 125.f;
}

GuiClient::~GuiClient() {
    if (receive_thread_.joinable()) {
        shared_data_.use_safely([&](SharedData& data) {
            if (data.stream_context) {
                data.stream_context->TryCancel();
            }
        });
        receive_thread_.join();
    }
}

void GuiClient::configure_gui(int, int h) {
    imgui::ScopedFont font(theme_->font);

    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, h), ImVec2(std::numeric_limits<float>::infinity(), h));
    ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Server Status: ");
    ImGui::SameLine();

    bool attempt_reconnect = false;

    shared_data_.use_safely([&](const SharedData& data) {
        if (data.connected_to_server) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Conneted");
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Connected");

            if (auto_gui_) {
                auto_gui_ = nullptr; // can remove this since we have no server
            }

            if (not data.error_messages.empty()) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Last Error: %s", data.error_messages.c_str());
            }

            if (ImGui::Button("Attempt Reconnect")) {
                attempt_reconnect = true;
            }

            ImGui::SameLine();
            ImGui::Text("(Make sure a server is running)");
        }
    });

    {
        char buf[1024];
        server_address_.copy(buf, server_address_.size());
        buf[server_address_.size()] = '\0';

        attempt_reconnect |= ImGui::InputText("Host", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);
        server_address_ = buf;
    }

    if (attempt_reconnect) {
        reset_channel();
    }

    if (auto_gui_) {
        ImGui::Separator();

        if (ImGui::CollapsingHeader("GUI Options")) {
            ImGui::Checkbox("Auto Update", &gui_options_->auto_update);
            ImGui::Checkbox("Display Type Names", &gui_options_->show_types);
            ImGui::Checkbox("Display Empty Fields", &gui_options_->display_empty_fields);
            ImGui::Checkbox("Headers Expanded by Default", &gui_options_->headers_expanded_by_default);
            ImGui::Checkbox("Display All oneof Messages", &gui_options_->display_all_oneof_messsages);
            ImGui::DragFloat("Label Width", &gui_options_->label_width, 1.f, 0.f, 500.f);
            ImGui::Checkbox("Strings Change on Enter Only", &gui_options_->strings_change_on_enter_only);
            ImGui::Checkbox("Display Only", &gui_options_->display_only);
        }

        auto_gui_->configure_gui(*gui_options_);
    }

    ImGui::End();

    //    ImGui::PopFont();
}

void GuiClient::reset_channel() {
    if (receive_thread_.joinable()) {
        shared_data_.use_safely([&](SharedData& data) {
            if (data.stream_context) {
                data.stream_context->TryCancel();
            }
        });
        receive_thread_.join();
    }

    stub_ = nullptr;
    channel_ = nullptr;

    channel_ = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
    stub_ = proj::proto::Server::NewStub(channel_);

    // Give the channel 5 chances to connect
    int max_connection_attempts = 5;
    int current_connection_attempt = 1;

    // TODO run this in a separate thread and send back connection state to the GUI
    do {
        int unused;
        grpc::CompletionQueue queue;

        bool ok;
        void* ptr = &unused;
        auto channel_state = channel_->GetState(true);

        if (channel_state != GRPC_CHANNEL_READY) {
            // never wait more than 15 seconds (super max upper bound we shouldn't actually reach)
            auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(15);
            channel_->NotifyOnStateChange(channel_state, deadline, &queue, &unused);

            std::cout << "Attempting to connect to '" << server_address_ << "' (attempt " << current_connection_attempt
                      << " of " << max_connection_attempts << ")." << std::endl;

            queue.Next(&ptr, &ok);
        }
    } while (current_connection_attempt++ < max_connection_attempts);

    shared_data_.unsafe_data().connected_to_server = (channel_->GetState(true) == GRPC_CHANNEL_READY);

    // TODO: Figure out how to make this generic and create a thread for each streamable function
    if (shared_data_.unsafe_data().connected_to_server) {
        auto_gui_ = std::make_unique<AutoGui>(channel_);

        receive_thread_ = std::thread([&] {
            google::protobuf::Empty unused;
            std::unique_ptr<grpc::ClientReader<proj::proto::Sink2>> stream;

            shared_data_.use_safely([&](SharedData& data) {
                data.stream_context = std::make_unique<grpc::ClientContext>();
                stream = stub_->stream_state2(data.stream_context.get(), unused);
            });

            proj::proto::Sink2 state;

            {
                std::ofstream graphvis_file("client_state.dot.ps");
                graphvis_file << util::graphvis_string(state);
            }

            while (stream->Read(&state)) {
                std::cout << "*******************************************\n";
                std::cout << "RECEIVED STATE: \n";
                std::cout << state.DebugString();
                std::cout << "*******************************************\n";
                std::cout << std::flush;

                std::ofstream graphvis_file("client_state.dot.ps");
                graphvis_file << util::graphvis_string(state);

                glfwPostEmptyEvent();
            }
            grpc::Status status = stream->Finish();

            shared_data_.use_safely([&](SharedData& data) {
                if (not status.ok()) {
                    data.error_messages = status.error_message();
                } else {
                    data.error_messages = "";
                }
                data.connected_to_server = false;
            });
            glfwPostEmptyEvent();
        });
    }
}

} // namespace proj
