#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <set>
#include "utils.hpp"
#include "commandhandler.hpp"


struct ID {
    long long timestamp = 0;
    int seq = 0;
};

struct Value {
    ID id;
    std::vector<std::string> data;
    bool operator< (const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator<=(const Value& other) const;
};

class DataBase {
public:

    // DatabaseKV interface
    void add_key(const std::string& key, const std::string& value, long long exp = -1);
    std::optional<std::string> get_key(const std::string& key);

    // DatabaseStream interface
    void add_stream (const std::string& key, const Value& value);
    std::optional<std::set<Value>> get_stream_set (const std::string& key);
    
private:

    friend class CommandHandler;
    
    ID StringToID (const std::string& id);
    Value CommandToValue (const std::vector<std::string>& Command);

    std::unordered_map<std::string, std::pair<std::string, long long>> DatabaseKV;
    std::unordered_map<std::string, std::set<Value>> DatabaseStream;
};

#endif // DATABASE_HPP
