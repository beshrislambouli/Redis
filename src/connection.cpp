#include "connection.hpp"
#include <iostream>

Connection::Connection(asio::io_context& io_context, Server* server, bool toMaster)
    : Socket_(io_context), Server_(server), CommandHandler_(std::make_shared<CommandHandler>(server, this)), toMaster(toMaster) {}

tcp::socket& Connection::get_socket() {
    return Socket_;
}

void Connection::init() {
    read_data();
}

void Connection::read_data() {
    auto self(shared_from_this());
    Socket_.async_read_some(
        asio::buffer(buffer_),
        [this, self](const asio::error_code& ec, std::size_t len) {
            if (!ec) {
                buffer_[len] = '\0'; // Null-terminate the received data
                CommandHandler_->handle_command(buffer_);
            } else {
                Err(ec);
            }
            read_data(); // Continue reading data
        }
    );
}

void Connection::write_data(const std::string& message) {
    std::cout << message << std::endl;
    if (toMaster) return; // Do not write if it's a master connection
    auto self(shared_from_this());
    asio::async_write(
        Socket_, asio::buffer(message),
        [this, self](const asio::error_code& ec, std::size_t /*len*/) {
            if (ec) Err(ec);
        }
    );
}
