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
#include <unordered_map>
#include <string>
#include <cstring>
#include <optional>
#include <climits>
#include <map>


using asio::ip::tcp;


const int kBufferSize = 1024;


//helpers
void Err (const asio::error_code& ec){
  std::cout << "Error: " << ec.message () << std::endl;
}

long long getime (int inf = 0){
  if (inf == -1) return 1e9;
  auto now = std::chrono::system_clock::now();
  auto now_in_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto value = now_in_ms.time_since_epoch();
  long long current_time_in_ms = value.count();
  return current_time_in_ms;
}


class DataBase {
public:
  DataBase () {
    std::cout << "started the db" << std::endl;
  }
  void add (const std::string& key, const std::string& value, long long exp = -1) {    
    exp = (exp == -1 ? getime(-1): getime() + exp);    
    database_.insert ( { key, {value,exp}});
  }
  
  std::optional <std::string> get (const std::string& key) {
    auto ans = database_ .find (key);
    if (ans == database_.end ()) return std::nullopt;
    if (ans->second.second > getime ()) {
      database_. erase (key);
      return std::nullopt;
    }
    return ans->second.first;
  }
private:
  std::unordered_map <std::string, std::pair <std::string, long long>> database_;
};


class Command_Handler {

public:
  Command_Handler (std::shared_ptr <DataBase> DataBase_): 
  DataBase_ (DataBase_)
  {}
  std::string handle_command (const std::string& Origin_command){
    ss.str(""), ss.clear();
    Origin_command_ = Origin_command;
    command_ = Command_Parser ();
    do_command();
    return ss.str();
  }

  std::vector<std::string> Command_Parser () {
    char type = Origin_command_ [0];
    std::vector <std::string> list;
    if ( type == '*') {
      int it = 4;
      while (it < Origin_command_ .size () ) {
          it ++;
          std::string llen = "";
          while (Origin_command_ [it] !='\r') {
            llen += Origin_command_ [it];
            it ++;
          } it += 2;
          int len = stoi (llen);
          std::string crnt = "";
          for (int i = 0 ; i < len ; i ++ ) {
            crnt += tolower (Origin_command_ [it]);
            it ++;
          } it +=2 ;
          list .push_back (crnt);
      }
    }
    return list;
  }



  void do_command () {
    std::string ty = command_[0];
    if ( ty == "ping" ) {
      ping_ ();
    }
    else if (ty == "echo"){
      echo_ ();
    }
    else if (ty == "get") {
      get_ ();
    }
    else if (ty == "set") {
      set_();
    }
  }
private:


  void ping_ () {
    ss << "+PONG\r\n";
  }
  void echo_ () {
    ss << "+" + command_ [1] + "\r\n";
  }

  void get_ () {
    std::optional <std::string> key = DataBase_->get (command_[1]);
    if (key.has_value()){
      std::string ans = std::move (key.value());
      ss << "$" << ans.size() << "\r\n" << ans << "\r\n";
    }
    else {
      ss << "$-1\r\n";
    }
  }

  void set_ () {
    ss << "+OK\r\n";
    DataBase_->add (command_[1], command_[2], (command_.size () > 3 ? std::stoi (command_[4]) : -1 ));
  }

  std::stringstream ss;
  std::string Origin_command_;
  std::vector<std::string> command_;
  std::shared_ptr <DataBase> DataBase_;
};


class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection (asio::io_context& io_context, std::shared_ptr <DataBase> DataBase_) :
  socket_ (io_context),
  command_handler_ (DataBase_)
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

          std::string reply = command_handler_.handle_command (buffer_);
          write_data (reply); // reply to client
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
  Command_Handler command_handler_;
};




class Server {
public:


  Server (asio::io_context& io_context) :
  io_context (io_context),
  acceptor_ (io_context, tcp::endpoint(tcp::v4(), 6379)),
  DataBase_ (std::make_shared <DataBase> ())
  {
    accept_connection ();  
  }


private:

  //init a newConnection and bind a client to it
  void accept_connection (){
    auto newConnection = std::make_shared <Connection> (io_context, DataBase_);
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
  std::shared_ptr <DataBase> DataBase_ ;
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
