// project
#include <client/cli_client.h>
#include <client/gui_client.h>

int main(int argc, const char* argv[]) {
    std::string server_address("0.0.0.0:50050");

    if (argc > 1) {
        server_address = argv[1];
    }

    if (argc > 2 and argv[2] == std::string("--cli")) {
        proj::CliClient client(server_address);
        client.run_loop();
    } else {
        proj::GuiClient client(server_address);
        client.run_loop();
    }

    return 0;
}
