#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include "server.hpp"
#include "commandhandler.hpp"

using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(asio::io_context& io_context, Server* server, bool toMaster = false);
    tcp::socket& get_socket();
    void init();
    void read_data();
    void write_data(const std::string& message);

private:
    tcp::socket Socket_;
    bool toMaster;
    char buffer_[1024];
    Server* Server_;
    std::shared_ptr<CommandHandler> CommandHandler_;
};

#endif // CONNECTION_HPP
