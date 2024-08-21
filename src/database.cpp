#include "database.hpp"
#include <iostream>


void DataBase::add_key(const std::string& key, const std::string& value, long long exp) {
    std::lock_guard<std::mutex> lock(KVMtx);
    exp = (exp == -1 ? getime(-1) : getime() + exp);
    std::cout << "to insert " << key << " " << value << std::endl;
    DatabaseKV [key] = {value, exp};
    std::cout << "done " << DatabaseKV [key] .first << std::endl;
}

std::optional<std::string> DataBase::get_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(KVMtx);
    auto ans = DatabaseKV.find(key);
    if (ans == DatabaseKV.end()) return std::nullopt;
    if (ans->second.second < getime()) {
        DatabaseKV.erase(key);
        return std::nullopt;
    }
    return ans->second.first;
}

bool ID::operator< (const ID& other) const {
    if (timestamp < other.timestamp) return true;
    if (timestamp == other.timestamp && seq < other.seq ) return true;
    return false;
}

bool ID::operator==(const ID& other) const {
    return (timestamp == other.timestamp && seq == other.seq );
}

bool ID::operator<=(const ID& other) const {
    return (*this == other || *this < other );
}

bool Value::operator< (const Value& other) const {
    return this->id < other.id;
}

bool Value::operator==(const Value& other) const {
    return this->id == other.id;
}

bool Value::operator<=(const Value& other) const {
    return this->id <= other.id;
}

void DataBase::add_stream (const std::string& key, const Value& value) {
    auto ans = DatabaseStream.find(key);
    if (ans == DatabaseStream.end ()) {
        std::set <Value> tmp;
        tmp .insert (value);
        DatabaseStream[key] = tmp;
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
    std::string key = Command [1];
    tmp .id = StringToID (key, Command[2]);
    std::vector <std::string> data; for (int i = 3 ; i < Command.size () ; i ++ ) data. push_back (Command [i]);
    tmp .data = data;

    return tmp;
}

ID DataBase::StringToID (const std::string& key, const std::string& id) {
    ID tmp;
    std::string timestamp, seq;
    auto p = SplitAt (id,'-');
    timestamp = p.first;
    seq = p.second;

    if (timestamp == "*") {
        tmp.timestamp = getime ();
        tmp.seq = 0;
    }
    else {
        tmp.timestamp = std::stol (timestamp);
        if (seq == "*") {
            std::optional<std::set<Value>> ss = get_stream_set (key);
            if (!ss.has_value ()) {
                tmp.seq = (int)(tmp.timestamp == 0);
            }
            else {
                std::set<Value> s = ss.value();
                auto it = s.end (); it --;
                if (it->id.timestamp == tmp.timestamp) {
                    tmp.seq = it->id.seq + 1;
                }
                else {
                    tmp.seq = 0;
                }
            }
        }
        else {
            tmp.seq = std::stoi (seq);
        }
    }
    return tmp;
}

int DataBase::validId (const std::string&key, const Value& value) {
    if ( !value.id.timestamp && !value.id.seq ) return -1;
    std::optional<std::set<Value>> ss = get_stream_set (key);
    if (!ss.has_value()) return true;
    std::set<Value> s = ss.value();
    auto it = s.end (); it --;
    return (it->id < value.id);
}

Value DataBase::CommandToRange (const std::string& r) {
    ID tmp;
    if (r == "-") {
        tmp.timestamp = 0;
        tmp.seq = 0; //don't care about {0,0}
    }
    else if ( r == "+" ) {
        tmp.timestamp = getime (-1);
        tmp.seq = 0; // don't care since max 
    }
    else {
        auto pt = SplitAt (r,'-');
        tmp.timestamp = std::stol (pt.first);
        tmp.seq = std::stoi (pt.second);
    }
    Value tmpp;
    tmpp .id = tmp;
    return tmpp;
}

std::vector <Value> DataBase::get_range (const std::string& key, Value& l, Value& r, int inclusive) {
    std::vector <Value> tmp;
    std::optional<std::set<Value>> ss = get_stream_set (key);
    if (ss.has_value()) {
        std::set<Value> s = ss.value ();
        auto it = (inclusive ? s.lower_bound (l) : s.upper_bound (l));
        for (; it != s.end () ; it ++ ) {
            if ( r < *it ) break;
            tmp .push_back (*it);
        }
    }
    return tmp;
}