// project
#include <server/server.h>
// standard
#include <iostream>

int main(int argc, const char* argv[]) {
    std::string server_address("0.0.0.0:50050");

    if (argc > 1) {
        server_address = argv[1];
    }

    {
        svr::Server server(server_address);

        std::cout << "Press enter to exit." << std::endl;
        std::cin.ignore();
        std::cout << "Exiting." << std::endl;
    }

    return 0;
}
