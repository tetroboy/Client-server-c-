#include "SecurityManager.h"

std::string SecurityManager::session_password = SecurityManager::generate_random_password();
std::unordered_map<boost::asio::ip::tcp::socket*, std::string> SecurityManager::session_keys;

std::string SecurityManager::generate_random_password(int length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    std::string password;
    for (int i = 0; i < length; ++i) {
        password += chars[dis(gen)];
    }
    return password;
}

std::string SecurityManager::generate_token(const std::string& data, const std::string& key) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), nullptr, nullptr);
    char md_string[65];
    for (int i = 0; i < 32; i++) {
        sprintf_s(&md_string[i * 2], 3, "%02x", (unsigned int)digest[i]);
    }
    return std::string(md_string);
}

std::string SecurityManager::get_session_password() {
    return session_password;
}

std::string SecurityManager::get_session_key(boost::asio::ip::tcp::socket* socket) {
    auto it = session_keys.find(socket);
    return (it != session_keys.end()) ? it->second : "";
}

void SecurityManager::set_session_key(boost::asio::ip::tcp::socket* socket, const std::string& key) {
    session_keys[socket] = key;
}