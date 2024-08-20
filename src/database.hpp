#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include "utils.hpp"

class DataBase {
public:
    DataBase();
    void add(const std::string& key, const std::string& value, long long exp = -1);
    std::optional<std::string> get(const std::string& key);
private:
    std::unordered_map<std::string, std::pair<std::string, long long>> database_;
};

#endif // DATABASE_HPP
