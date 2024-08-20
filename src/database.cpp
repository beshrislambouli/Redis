#include "database.hpp"
#include <iostream>



DataBase::DataBase() {
    std::cout << "started the db" << std::endl;
}

void DataBase::add(const std::string& key, const std::string& value, long long exp) {
    exp = (exp == -1 ? getime(-1) : getime() + exp);
    database_.insert({ key, { value, exp } });
}

std::optional<std::string> DataBase::get(const std::string& key) {
    auto ans = database_.find(key);
    if (ans == database_.end()) return std::nullopt;
    if (ans->second.second < getime()) {
        database_.erase(key);
        return std::nullopt;
    }
    return ans->second.first;
}
