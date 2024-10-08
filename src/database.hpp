#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <set>
#include <mutex>
#include "utils.hpp"
#include "commandhandler.hpp"


struct ID {
    long long timestamp = 0;
    int seq = 0;
    bool operator< (const ID& other) const;
    bool operator==(const ID& other) const;
    bool operator<=(const ID& other) const;
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
    std::vector <Value> get_range (const std::string& key, Value& l, Value& r, int inclusive = 1);
private:

    friend class CommandHandler;
    
    

    ID StringToID (const std::string&key, const std::string& id);
    Value CommandToValue (const std::vector<std::string>& Command);
    Value CommandToRange (const std::string& r);
    int validId (const std::string&key, const Value& value);

    std::mutex KVMtx;
    std::mutex StreamMtx;
    std::unordered_map<std::string, std::pair<std::string, long long>> DatabaseKV;
    std::unordered_map<std::string, std::set<Value>> DatabaseStream;
};

#endif // DATABASE_HPP
