#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include "database.hpp"
#include "connection.hpp"
#include "server.hpp"
#include "serverinfo.hpp"

class Server;
class Connection;

class CommandHandler {
public:
    CommandHandler(Server* Server_, Connection* Connection_);
    void handle_command(const std::string& OriginCommand);

private:
    std::vector<std::vector<std::string>> CommandParser();
    void do_command();
    void ping_();
    void echo_();
    void get_();
    void set_();
    void incr_();
    void info_();
    void replconf_();
    void psync_();
    void getack_ ();
    void type_ ();
    void xadd_ ();
    void xrange_ ();
    void xread_ ();

    void propagate_();
    void Reply();

    std::stringstream ss;
    std::string OriginCommand_;
    std::vector<std::vector<std::string>> Commands_;
    std::vector<std::string> Command_;
    Server* Server_;
    Connection* Connection_;
    bool exception = false;
    int offset = 0;

};

#endif // COMMANDHANDLER_HPP
