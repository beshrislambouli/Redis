#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
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
using asio::ip::tcp;


const int kBufferSize = 1024;

void Err (const asio::error_code& ec){
  std::cout << "Error: " << ec.message () << std::endl;
}

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection (asio::io_context& io_context) :
  socket_ (io_context)
  {}

  //getters
  tcp::socket& get_socket () { return socket_ ; }

  void init () { // init the connection
    read_data ();
  }

  void read_data () {
    auto self(shared_from_this());
    socket_.async_read_some ( // read from client
      asio::buffer(buffer_),
      [this, self] (const asio::error_code& ec, std::size_t len) {
        if (!ec) {
          buffer_ [len] = '\0';
          std::cout << buffer_ << std::endl;
          
          std::string message = "+PONG\r\n";
          write_data (message); // reply to client
        }
        else Err (ec);
      }
    );
  }

  void write_data (const std::string& message){
    auto self (shared_from_this());

    asio::async_write ( // reply to client
      socket_ , asio::buffer (message),
      [this,self] (const asio::error_code& ec, std::size_t len) {
        if (!ec) {
          read_data (); // read from client
        }
        else Err (ec);
      }
    );
  }

private:
  tcp::socket socket_; // the connection client
  char buffer_ [kBufferSize]; // buffer for input
};

class Server {
public:


  Server (asio::io_context& io_context) :
  io_context (io_context),
  acceptor_ (io_context, tcp::endpoint(tcp::v4(), 6379))
  {
    accept_connection ();  
  }


private:

  //init a newConnection and bind a client to it
  void accept_connection (){
    auto newConnection = std::make_shared <Connection> (io_context);
    acceptor_.async_accept (
      newConnection->get_socket(),
      [this, newConnection] (const asio::error_code& ec) {
        if (!ec) {
          newConnection -> init ();
        }
        else Err (ec);
        accept_connection (); // Accept the next connection
      }
    );
  }

  asio::io_context& io_context;
  tcp::acceptor acceptor_;
};


int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  try {
    asio::io_context io_context; 
    Server server (io_context);
    io_context.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
