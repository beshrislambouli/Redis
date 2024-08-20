#ifndef SERVERINFO_HPP
#define SERVERINFO_HPP

#include <string>

struct ServerInfo {
    int port = 6379; // default port
    bool is_master = true;
    std::string MASTER_HOST;
    long long MASTER_PORT;
};

#endif // SERVERINFO_HPP
