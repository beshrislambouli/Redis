#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <vector>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/io_context.hpp>
#include <asio/completion_condition.hpp>
#include <asio/connect.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/registered_buffer.hpp>
#include <asio/streambuf.hpp>
#include <asio/write.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/streambuf.hpp>
#include <asio/read_until.hpp>
#include <asio/use_future.hpp>
#include "connection.hpp"
#include "database.hpp"
#include "utils.hpp"
#include "serverinfo.hpp"

class Connection;  // Forward declaration
class CommandHandler;  // Forward declaration
class DataBase;


class Server : public std::enable_shared_from_this<Server> {
public:
    Server(asio::io_context& io_context, const ServerInfo& serverInfo);

private:
    void accept_connection();
    void Connect_to_Master();

    friend class Connection;
    friend class CommandHandler;

    asio::io_context& io_context;
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<DataBase> DataBase_;
    std::shared_ptr<ServerInfo> ServerInfo_;
    std::shared_ptr<Connection> MasterConnection_;
    std::vector<Connection*> Replicas_;
};

#endif // SERVER_HPP
