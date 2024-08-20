#include "database.hpp"
#include <iostream>


void DataBase::add_key(const std::string& key, const std::string& value, long long exp) {
    exp = (exp == -1 ? getime(-1) : getime() + exp);
    DatabaseKV.insert({ key, { value, exp } });
}

std::optional<std::string> DataBase::get_key(const std::string& key) {
    auto ans = DatabaseKV.find(key);
    if (ans == DatabaseKV.end()) return std::nullopt;
    if (ans->second.second < getime()) {
        DatabaseKV.erase(key);
        return std::nullopt;
    }
    return ans->second.first;
}

bool Value::operator< (const Value& other) const {
    if (id.timestamp < other.id.timestamp) return true;
    if (id.timestamp == other.id.timestamp && id.seq < other.id.seq ) return true;
    return false;
}

bool Value::operator==(const Value& other) const {
    return (id.timestamp == other.id.timestamp && id.seq == other.id.seq );
}

bool Value::operator<=(const Value& other) const {
    return (*this == other || *this < other );
}

void DataBase::add_stream (const std::string& key, const Value& value) {
    auto ans = DatabaseStream.find(key);
    if (ans == DatabaseStream.end ()) {
        std::set <Value> tmp;
        tmp .insert (value);
        DatabaseStream.insert ({ key, tmp });
    }
    else {
        ans -> second .insert (value);
    }
}

std::optional<std::set<Value>> DataBase::get_stream_set (const std::string& key) {
    auto ans = DatabaseStream.find(key);
    if (ans == DatabaseStream.end()) return std::nullopt;
    return ans->second;
}

Value DataBase::CommandToValue (const std::vector<std::string>& Command) {
    //Command [0] = command type, Command [1] = key, Command [2] = ID, Command [...] = data
    Value tmp;

    tmp .id = StringToID (Command[2]);

    std::vector <std::string> data; for (int i = 3 ; i < Command.size () ; i ++ ) data. push_back (Command [i]);
    tmp .data = data;

    return tmp;
}

ID DataBase::StringToID (const std::string& id) {
    ID tmp;
    std::string timestamp, seq;

    int t = 0 ;
    for ( int i = 0 ; i < id.size () ; i ++ ) {
        if (id [i] == '-') {
            t = 1 ;
            continue;
        }
        if (!t) {
            timestamp += id [i];
        } else {
            seq += id [i];
        }
    }

    tmp.timestamp = std::stol (timestamp);
    tmp.seq = std::stoi (seq);

    return tmp;
}

