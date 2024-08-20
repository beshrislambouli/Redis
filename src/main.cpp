#include <iostream>
#include <memory>
#include <asio/io_context.hpp>
#include "server.hpp"
#include "utils.hpp"
#include "serverinfo.hpp"

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Parse the input arguments to configure the server
    ServerInfo serverInfo = ArgParser(argc, argv);

    try {
        asio::io_context io_context;
        Server server (io_context, serverInfo);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
