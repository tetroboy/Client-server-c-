#pragma once
#include "Libs.h"

class SecurityManager {
public:
    static std::string generate_random_password(int length = 16);
    static std::string generate_token(const std::string& data, const std::string& key);
    static std::string get_session_password();
    static std::string get_session_key(boost::asio::ip::tcp::socket* socket);
    static void set_session_key(boost::asio::ip::tcp::socket* socket, const std::string& key);

private:
    static std::string session_password;
    static std::unordered_map<boost::asio::ip::tcp::socket*, std::string> session_keys;
};