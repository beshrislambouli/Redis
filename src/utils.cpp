#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <limits>
#include <stdexcept>

ServerInfo ArgParser(int argc, char **argv) {
    ServerInfo ret;
    for (int i = 0; i < argc; i++) {
        std::string arg = std::string(argv[i]);
        if (arg == "--port") {
            ret.port = std::stoi(argv[i + 1]);
        }
        if (arg == "--replicaof") {
            std::istringstream iss(argv[i + 1]);
            std::string host;
            long long port;
            iss >> host >> port;
            ret.MASTER_HOST = host;
            ret.MASTER_PORT = port;
            ret.is_master = false;
        }
    }
    return ret;
}

void Err(const asio::error_code& ec) {
    std::cerr << "Error: " << ec.message() << std::endl;
}

long long getime(int inf) {
    if (inf == -1) return LLONG_MAX;
    auto now = std::chrono::system_clock::now();
    auto now_in_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_in_ms.time_since_epoch();
    return value.count();
}

std::pair <std::string, std::string> SplitAt (const std::string& s, char c) {
    std::string s1,s2;
    s1 = s2 = "";
    int t = 0 ;
    for ( int i = 0 ; i < s.size () ; i ++ ) {
        if (s [i] == c) {
            t = 1 ;
            continue;
        }
        if (!t) {
            s1 += s [i];
        } else {
            s2 += s [i];
        }
    }
    return {s1,s2};
}

