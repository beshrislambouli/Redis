#include "commandhandler.hpp"
#include <cctype>
#include <iostream>

CommandHandler::CommandHandler(Server* Server_, Connection* Connection_)
    : Server_(Server_), Connection_(Connection_) {}

void CommandHandler::handle_command(const std::string& OriginCommand) {
    OriginCommand_ = std::move(OriginCommand);
    offset += OriginCommand_.size ();
    Commands_ = CommandParser();
    std::cout <<"-----------------------" << std::endl;
    for (auto command: Commands_) {
        for (auto u :command) {
            std::cout << u << " " ;
        }
        std::cout << std::endl;
    }
    std::cout <<"-----------------------" << std::endl;

    for (auto& u: Commands_) {
        Command_ = u;
        do_command();
        Reply();
    }
}


std::vector<std::vector<std::string>> CommandHandler::CommandParser() {
    std::vector<std::vector<std::string>> commands;
    size_t pos = 0;

    auto read_line = [&]() -> std::string {
        size_t end_pos = OriginCommand_.find('\n', pos);
        if (end_pos == std::string::npos) end_pos = OriginCommand_.size();
        std::string line = OriginCommand_.substr(pos, end_pos - pos);
        pos = end_pos + 1;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Remove the trailing \r
        }
        return line;
    };

    while (pos < OriginCommand_.size()) {
        std::string line = read_line();
        if (line.empty()) continue; // Skip empty lines

        if (line[0] == '*') {
            int repeated = std::stoi(line.substr(1));
            std::vector<std::string> command;
            for (int i = 0; i < repeated; ++i) {
                line = read_line();
                if (line.empty()) {
                    std::cerr << "Failed to parse command = " << OriginCommand_ << "\n";
                    return {};
                }
                if (line[0] == '$') {
                    int str_len = std::stoi(line.substr(1));
                    if (str_len < 0) {
                        std::cerr << "Invalid string length = " << OriginCommand_ << "\n";
                        return {};
                    }
                    std::string bulk_str = OriginCommand_.substr(pos, str_len);
                    pos += str_len;
                    command.push_back(std::move(bulk_str));
                    pos += 2; // Skip CRLF
                } else {
                    std::cerr << "Unexpected line format = " << OriginCommand_ << "\n";
                    return {};
                }
            }
            commands.push_back(std::move(command));
        } else if (line[0] == '$') {
            int str_len = std::stoi(line.substr(1));
            if (str_len < 0) {
                std::cerr << "Invalid string length = " << OriginCommand_ << "\n";
                return {};
            }
            std::string bulk_str = OriginCommand_.substr(pos, str_len);
            pos += str_len;
            pos += 2; // Skip CRLF
            commands.emplace_back(1, std::move(bulk_str));
        } else {
            std::cerr << "Incorrect format, command = " << OriginCommand_ << "\n";
            return {};
        }
    }

    for (auto& command: commands) {
        for (auto& u :command) {
            for (auto& v: u) {
                v = std::tolower (v);
            }
        }
    }

    return commands;
}

void CommandHandler::do_command() {
    std::string ty = Command_[0];
    if (ty == "ping") {
        ping_();
    } else if (ty == "echo") {
        echo_();
    } else if (ty == "get") {
        get_();
    } else if (ty == "set") {
        set_();
    } else if (ty == "info") {
        info_();
    } else if (ty == "replconf") {
        if (Command_[1] == "getack") getack_ ();
        else replconf_();
    } else if (ty == "psync") {
        psync_();
    } else if (ty == "type") {
        type_();
    } else if (ty == "xadd") {
        xadd_();
    } else if (ty == "xrange") {
        xrange_ ();
    } else if (ty == "xread") {
        xread_ ();
    } else if (ty == "incr") {
        incr_ ();
    }
}

void CommandHandler::ping_() {
    ss << "+PONG\r\n";
}

void CommandHandler::echo_() {
    ss << "+" + Command_[1] + "\r\n";
}

void CommandHandler::get_() {
    std::optional<std::string> value = Server_->DataBase_->get_key(Command_[1]);
    if (value.has_value()) {
        std::string ans = value.value();
        ss << "$" << ans.size() << "\r\n" << ans << "\r\n";
    } else {
        ss << "$-1\r\n";
    }
}

void CommandHandler::set_() {
    ss << "+OK\r\n";
    Server_->DataBase_->add_key(Command_[1], Command_[2], (Command_.size() > 3 ? std::stoi(Command_[4]) : -1));
    propagate_();
}

void CommandHandler::incr_ () {
    const std::string& key = Command_ [1];
    std::optional<std::string> value = Server_->DataBase_->get_key(Command_[1]);
    if (value.has_value ()) {
        if (! isNumber (value.value())) {
            ss << "-ERR value is not an integer or out of range\r\n";
            return ;
        }
        int num = std::stoi (value.value ()) + 1;
        Server_->DataBase_->add_key(key, std::to_string (num));
        ss << ":" << num << "\r\n";
    }
    else {
        Server_->DataBase_->add_key(key, "1");
        ss << ":1\r\n";
    }
    propagate_();
}

void CommandHandler::info_() {
    std::string reply = "";
    if (Server_->ServerInfo_->is_master) {
        reply = "role:master\nmaster_replid:8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb\nmaster_repl_offset:0\n";
    } else {
        reply = "role:slave\n";
    }
    ss << "$" << reply.size() << "\r\n" << reply << "\r\n";
}

void CommandHandler::replconf_() {
    ss << "+OK\r\n";
}

void CommandHandler::psync_() {
    ss << "+FULLRESYNC 8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb 0\r\n";
    Reply();
    ss << "$66\r\n\x52\x45\x44\x49\x53\x30\x30\x31\x31\xfa\x09\x72\x65\x64\x69\x73\x2d\x76\x65\x72\x05\x37\x2e\x32\x2e\x30\xfa\x0a\x72\x65\x64\x69\x73\x2d\x62\x69\x74\x73\xc0\x40\xfa\x05\x63\x74\x69\x6d\x65\xc2\x6d\x08\xbc\x65\xfa\x08\x75\x73\x65\x64\x2d\x6d\x65\x6d\xc2\xb0\xc4\x10\x00\xfa\x08\x61\x6f\x66\x2d\x62\x61\x73\x65\xc0\x00\xff\xf0\x6e\x3b\xfe\xc0\xff\x5a\xa2";
    Server_->Replicas_.push_back(Connection_);
}

void CommandHandler::getack_ () {
    // -37 because of REPLCONF GETACK *
    ss << "*3\r\n$8\r\nREPLCONF\r\n$3\r\nACK\r\n"; 
    ss << "$" << std::to_string (offset-37).size() << "\r\n" << offset-37 << "\r\n";
    exception = true;
}

void CommandHandler::type_ () {
    std::optional<std::string> value1 = Server_->DataBase_->get_key(Command_[1]);
    std::optional<std::set<Value>> value2 = Server_->DataBase_->get_stream_set(Command_[1]);
    if (value1.has_value()) {
        ss << "+string\r\n";
    } else if (value2.has_value()) {
        ss << "+stream\r\n";
    } else {
        ss << "+none\r\n";
    }
}

void CommandHandler::xadd_ () {
    Value value = Server_->DataBase_->CommandToValue (Command_);
    int IsValid = Server_->DataBase_->validId (Command_[1], value);
    if (IsValid == 0) {
        ss << "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return ;
    } else if (IsValid == -1) {
        ss << "-ERR The ID specified in XADD must be greater than 0-0\r\n";
        return ;
    }
    std::string id = std::to_string (value.id.timestamp) + "-" + std::to_string (value.id.seq);
    ss << "$" << id.size () << "\r\n" << id << "\r\n";
    Server_->DataBase_->add_stream (Command_[1], value);
}

void CommandHandler::xrange_ () {
    std::string& key = Command_ [1];
    Value l = Server_->DataBase_->CommandToRange (Command_ [2]);
    Value r = Server_->DataBase_->CommandToRange (Command_ [3]);
    std::vector <Value> values = Server_->DataBase_->get_range (key,l,r);
    ss << "*" << values.size () <<  "\r\n";
    for (auto value: values) {
        ss << "*2\r\n";
        std::string id = std::to_string (value.id.timestamp) + "-" + std::to_string (value.id.seq);
        ss << "$" << id.size () << "\r\n" << id << "\r\n";
        ss << "*" << value.data.size () << "\r\n";
        for (auto d: value.data) {
            ss << "$" << d.size () << "\r\n" << d << "\r\n";
        }
    }
}

void CommandHandler::xread_ () {
    int num_streams = Command_.size () / 2 - 1 ;
    ss << "*" << num_streams << "\r\n";
    for ( int i = 2 ; i < 2 + num_streams ; i ++ ) {
        std::string& key = Command_ [i];
        Value l = Server_->DataBase_->CommandToRange (Command_ [i+num_streams]);
        Value r = Server_->DataBase_->CommandToRange ("+");
        std::vector <Value> values = Server_->DataBase_->get_range (key,l,r, 0);
        ss << "*2\r\n";
        ss << "$" << key.size () << "\r\n" << key << "\r\n";
        ss << "*" << values.size () <<  "\r\n";
        for (auto value: values) {
            ss << "*2\r\n";
            std::string id = std::to_string (value.id.timestamp) + "-" + std::to_string (value.id.seq);
            ss << "$" << id.size () << "\r\n" << id << "\r\n";
            ss << "*" << value.data.size () << "\r\n";
            for (auto d: value.data) {
                ss << "$" << d.size () << "\r\n" << d << "\r\n";
            }
        }
    }
}

void CommandHandler::propagate_() {
    for (auto& replica : Server_->Replicas_) {
        replica->write_data(OriginCommand_);
    }
}

void CommandHandler::Reply() {
    Connection_ -> write_data(ss.str(), exception);
    ss.str(""); ss.clear();
    exception = false;
}
