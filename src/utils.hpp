#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <chrono>
#include <asio/error_code.hpp>
#include "serverinfo.hpp"

// Utility function to parse command-line arguments
ServerInfo ArgParser(int argc, char **argv);

// Utility function to handle errors
void Err(const asio::error_code& ec);

// Utility function to get current time in milliseconds
long long getime(int inf = 0);

#endif // UTILS_HPP
