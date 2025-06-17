#include <boost/asio.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include "openssl/hmac.h"
#include "openssl/sha.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;

std::string generateToken(const std::string& data, const std::string& key) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), NULL, NULL);
    char mdString[65]; 
    for (int i = 0; i < 32; i++) {
        sprintf_s(&mdString[i * 2], 3, "%02x", (unsigned int)digest[i]); 
    }
    return std::string(mdString);
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234));
        std::cout << "Connected to server. Type commands (CPU usage, RAM usage, Disk usage, PID list, kill_process <PID>, Read_file <path>, Show_files <path>, Delete_file <path>, exit)\n";

        // Авторизація
        std::cout << "Enter session password from server console: ";
        std::string password;
        std::getline(std::cin, password);
        std::string authMessage = "auth " + password + "\n";
        boost::asio::write(socket, boost::asio::buffer(authMessage));

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');
        std::istream stream(&buffer);
        std::string response;
        std::getline(stream, response);
        std::cout << "Auth response: " << response << std::endl;

        json j = json::parse(response);
        if (j["status"] != "success") {
            std::cerr << "Authentication failed." << std::endl;
            socket.close();
            return 1;
        }
        std::string token = j["token"].get<std::string>();
        std::cout << "Authenticated with new token: " << token << std::endl;

        while (true) {
            std::cout << "> ";

            std::string command;
            std::getline(std::cin, command);
            command += "\n";
            std::cout << "Input command: " << command << std::endl;
            if (command == "exit\n") break;

            // Генерація токена для команди
            std::string commandWithoutNewline = command.substr(0, command.length() - 1);
            commandWithoutNewline.erase(commandWithoutNewline.find_last_not_of(" \n\r\t") + 1);
            commandWithoutNewline.erase(0, commandWithoutNewline.find_first_not_of(" \n\r\t"));
            std::string generatedToken = generateToken(commandWithoutNewline, token);
            std::cout << "Generated token: " << generatedToken << std::endl;
            std::string signedMessage = commandWithoutNewline + " token:" + generatedToken + "\n";
            boost::asio::write(socket, boost::asio::buffer(signedMessage));

            try {
                boost::asio::streambuf response;
                boost::asio::read_until(socket, response, '\n');

                std::istream stream(&response);
                json reply;
                stream >> reply;

                std::string json_str = reply.dump(4);
                std::cout << "Server response:\n" << json_str << "\n";
            }
            catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::eof) {
                    std::cerr << "Server closed connection. Reconnecting...\n";
                    socket.close();
                    socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234));
                    // Повторна авторизація не реалізована
                }
                else {
                    throw;
                }
            }
        }

        if (socket.is_open()) {
            socket.shutdown(tcp::socket::shutdown_both);
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}