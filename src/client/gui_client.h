#pragma once

#include "client/gui.h"
#include "util/atomic_data.h"

#include <proj/server.grpc.pb.h>
#include <grpcpp/channel.h>
#include <thread>

namespace proj {

class AutoGui;
struct GuiOptions;
class Theme;

class GuiClient : public Gui {
public:
    explicit GuiClient(std::string server_address);
    ~GuiClient() override;

    void configure_gui(int w, int h) override;

    //    void dispatch_action(const proj::proto::Actions& action);

private:
    std::string server_address_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<proj::proto::Server::Stub> stub_;

    std::thread receive_thread_;

    struct SharedData {
        std::unique_ptr<grpc::ClientContext> stream_context = {};
        bool connected_to_server = false;
        std::string error_messages = {};
    };

    util::AtomicData<SharedData> shared_data_;

    std::unique_ptr<GuiOptions> gui_options_;
    std::unique_ptr<AutoGui> auto_gui_;

    std::unique_ptr<Theme> theme_;

    void reset_channel();
};

} // namespace proj