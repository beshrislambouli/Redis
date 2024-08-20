#include "commandhandler.hpp"
#include <cctype>
#include <iostream>

CommandHandler::CommandHandler(Server* Server_, Connection* Connection_)
    : Server_(Server_), Connection_(Connection_) {}

void CommandHandler::handle_command(const std::string& OriginCommand) {
    OriginCommand_ = std::move(OriginCommand);
    Command_ = CommandParser();
    do_command();
    Reply();
}

std::vector<std::string> CommandHandler::CommandParser() {
    char type = OriginCommand_[0];
    std::vector<std::string> list;
    if (type == '*') {
        int it = 4;
        while (it < OriginCommand_.size()) {
            it++;
            std::string llen = "";
            while (OriginCommand_[it] != '\r') {
                llen += OriginCommand_[it];
                it++;
            }
            it += 2;
            int len = std::stoi(llen);
            std::string crnt = "";
            for (int i = 0; i < len; i++) {
                crnt += std::tolower(OriginCommand_[it]);
                it++;
            }
            it += 2;
            list.push_back(crnt);
        }
    }
    return list;
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
        replconf_();
    } else if (ty == "psync") {
        psync_();
    }
}

void CommandHandler::ping_() {
    ss << "+PONG\r\n";
}

void CommandHandler::echo_() {
    ss << "+" + Command_[1] + "\r\n";
}

void CommandHandler::get_() {
    std::optional<std::string> value = Server_->DataBase_->get(Command_[1]);
    if (value.has_value()) {
        std::string ans = value.value();
        ss << "$" << ans.size() << "\r\n" << ans << "\r\n";
    } else {
        ss << "$-1\r\n";
    }
}

void CommandHandler::set_() {
    ss << "+OK\r\n";
    Server_->DataBase_->add(Command_[1], Command_[2], (Command_.size() > 3 ? std::stoi(Command_[4]) : -1));
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

void CommandHandler::propagate_() {
    for (auto& replica : Server_->Replicas_) {
        replica->write_data(OriginCommand_);
    }
}

void CommandHandler::Reply() {
    Connection_->write_data(ss.str());
    ss.str(""); ss.clear();
}
