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
  if (inf == -1) return LLONG_MAX;
  auto now = std::chrono::system_clock::now();
  auto now_in_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto value = now_in_ms.time_since_epoch();
  long long current_time_in_ms = value.count();
  return current_time_in_ms;
}



struct ServerInfo {
  int port = 6379; //default port
  bool is_master = 1;
  std::string MASTER_HOST;
  long long MASTER_PORT;
};

ServerInfo Arg_Parser (int argc, char **argv) {
  ServerInfo ret;
  for ( int i = 0 ; i < argc ; i ++ ) {
    std::string arg = std::string (argv[i]);
    if (arg == "--port") {
      ret.port = std::stoi (argv[i+1]);
    }
    if (arg == "--replicaof") {
      std::istringstream iss (argv [i+1]);
      std::string host; iss >> host;
      long long port; iss >> port;
      ret .MASTER_HOST = host;
      ret .MASTER_PORT = port;
      ret .is_master = false;
    }
  }
  return ret;
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
    if (ans->second.second < getime ()) {
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
  Command_Handler (std::shared_ptr <DataBase> DataBase_, std::shared_ptr <ServerInfo> ServerInfo_): 
  DataBase_ (std::move (DataBase_)),
  ServerInfo_ (std::move(ServerInfo_))
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
    else if (ty == "info") {
      info_ ();
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
    std::cout << "here " << std::endl;
    std::optional <std::string> value = DataBase_->get (command_[1]);
    if (value.has_value()){
      std::string ans = value.value();
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

  void info_ () {
    std::string reply = "";
    if (ServerInfo_ -> is_master) {
      reply = "role:master\nmaster_replid:8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb\nmaster_repl_offset:0\n";
    }
    else {
      reply = "role:slave\n";
    }
    ss << "$" << reply.size () << "\r\n" << reply << "\r\n";
  }

  std::stringstream ss;
  std::string Origin_command_;
  std::vector<std::string> command_;
  std::shared_ptr <DataBase> DataBase_;
  std::shared_ptr <ServerInfo> ServerInfo_;
};


class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection (asio::io_context& io_context, std::shared_ptr <DataBase> DataBase_, std::shared_ptr <ServerInfo> ServerInfo_) :
  socket_ (io_context),
  command_handler_ (DataBase_, ServerInfo_)
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
          std::string reply = command_handler_.handle_command (buffer_);
          write_data (reply); // reply to client
        }
        else Err (ec);
      }
    );
  }

  void write_data (const std::string& message) {
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


  Server (asio::io_context& io_context, const ServerInfo& ServerInfo__) :
  io_context (io_context),
  acceptor_ (io_context, tcp::endpoint(tcp::v4(), ServerInfo__.port)),
  DataBase_ (std::make_shared <DataBase> ()),
  ServerInfo_ (std::make_shared<ServerInfo>(ServerInfo__))
  {
    if (! ServerInfo_ -> is_master) {
      Connect_to_Master ();
    }
    accept_connection ();  
  }


private:

  //init a newConnection and bind a client to it
  void accept_connection (){
    std::cout <<"accepting" << std::endl;
    auto newConnection = std::make_shared <Connection> (io_context, DataBase_, ServerInfo_);
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

  void Connect_to_Master () {
    MasterConnection_ = std::make_shared<Connection>(io_context, DataBase_, ServerInfo_);
    auto &socket = MasterConnection_->get_socket();
    tcp::resolver resolver(io_context);
    auto endpoint = resolver.resolve(ServerInfo_->MASTER_HOST, std::to_string(ServerInfo_->MASTER_PORT));
    asio::error_code ec;
    asio::connect(socket, endpoint, ec);

    std::string message;
    asio::streambuf reply;
    std::string response;

    
    message = "*1\r\n$4\r\nPING\r\n";
    asio::write(socket, asio::buffer(message), ec);
    asio::read_until(socket, reply, "\r\n");
    
    response ={asio::buffers_begin(reply.data()), asio::buffers_begin(reply.data()) + bytes - 2};
    reply.consume(bytes);
    std::cout << "Received: " << response << std::endl;


    message = "*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n6380\r\n"; // TODO: change to real port
    asio::write(socket, asio::buffer(message), ec);
    asio::read_until(socket, reply, "\r\n");

    response ={asio::buffers_begin(reply.data()), asio::buffers_begin(reply.data()) + bytes - 2};
    reply.consume(bytes);
    std::cout << "Received: " << response << std::endl;

    message = "*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n";
    asio::write(socket, asio::buffer(message), ec);
    asio::read_until(socket, reply, "\r\n");

    response ={asio::buffers_begin(reply.data()), asio::buffers_begin(reply.data()) + bytes - 2};
    reply.consume(bytes);
    std::cout << "Received: " << response << std::endl;


    std::string message2 = "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n";
    asio::write(socket, asio::buffer(message2), ec);
    asio::read_until(socket, reply, "\r\n");

    response ={asio::buffers_begin(reply.data()), asio::buffers_begin(reply.data()) + bytes - 2};
    reply.consume(bytes);
    std::cout << "Received: " << response << std::endl;

    
  }

  asio::io_context& io_context;
  tcp::acceptor acceptor_;
  std::shared_ptr <DataBase> DataBase_ ;
  std::shared_ptr <ServerInfo> ServerInfo_;
  std::shared_ptr <Connection> MasterConnection_;
};


int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  

  try {
    asio::io_context io_context; 
    Server server (io_context, Arg_Parser (argc, argv));
    io_context.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}