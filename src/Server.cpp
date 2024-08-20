#include "server.hpp"
#include "connection.hpp"
#include "commandhandler.hpp"
#include <iostream>
#include <asio/connect.hpp>

Server::Server(asio::io_context& io_context, const ServerInfo& serverInfo)
    : io_context(io_context),
      acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), serverInfo.port)),
      DataBase_(std::make_shared<DataBase>()),
      ServerInfo_(std::make_shared<ServerInfo>(serverInfo)) {
    if (!ServerInfo_->is_master) {
        Connect_to_Master();
    }
    accept_connection();
}

void Server::accept_connection() {
    std::cout << "Accepting connection" << std::endl;
    std::shared_ptr<Connection> newConnection = std::make_shared<Connection>(io_context, this);
    acceptor_.async_accept(
        newConnection->get_socket(),
        [this, newConnection](const asio::error_code& ec) {
            if (!ec) {
                newConnection->init();
            } else {
                Err(ec);
            }
            accept_connection(); // Accept the next connection
        }
    );
}

void Server::Connect_to_Master() {
    MasterConnection_ = std::make_shared<Connection>(io_context, this, true);
    auto& socket = MasterConnection_->get_socket();
    asio::ip::tcp::resolver resolver(io_context);
    auto endpoint = resolver.resolve(ServerInfo_->MASTER_HOST, std::to_string(ServerInfo_->MASTER_PORT));
    asio::error_code ec;
    asio::connect(socket, endpoint, ec);

    std::vector<std::string> messages = {
        "*1\r\n$4\r\nPING\r\n",
        "*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n6380\r\n",
        "*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n",
        "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n"
    };
    asio::streambuf reply;

    for (const std::string& message : messages) {
        asio::write(socket, asio::buffer(message), ec);
        int bytes = asio::read_until(socket, reply, "\r\n"); //the reply won't have \r\n in the middle
        reply.consume(bytes);
    }
    
    int bytes = asio::read_until (socket, reply, "\r\n"); // len \r\n then the rdb and getack
    int LenRDB = std::stoi ( std::string (
                                asio::buffers_begin(reply.data()) + 1, //not the $
                                asio::buffers_begin(reply.data()) + bytes - 2) // not the \r\n
                          ) ;

    reply.consume(bytes); //consume the len
    reply.consume(LenRDB); //consume the rdb

    std::string getack {   
                            asio::buffers_begin(reply.data()),
                            asio::buffers_end(reply.data())
                        };
    MasterConnection_->init();
    MasterConnection_-> CommandHandler_ -> handle_command (getack);
}

