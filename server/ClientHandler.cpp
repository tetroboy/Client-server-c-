#include "ClientHandler.h"


void ClientHandler::handle_client(boost::asio::ip::tcp::socket socket) {
    try {
        bool is_authorized = false;
        std::string session_key;

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');

        std::istream stream(&buffer);
        std::string message;
        std::getline(stream, message);
        std::cout << "Received initial message: " << message << std::endl;

        if (message.find("auth ") == 0) {
            std::string received_password = message.substr(5);
            if (received_password == SecurityManager::get_session_password()) {
                is_authorized = true;
                std::cout << "Client authorized successfully." << std::endl;
                Logger::log_action("Client authorized successfully");

                session_key = SecurityManager::generate_random_password(32);
                SecurityManager::set_session_key(&socket, session_key);
                std::string auth_response = "{\"status\":\"success\",\"message\":\"Authorized\",\"token\":\"" + session_key + "\"}\n";
                std::cout << "Sent token: " << session_key << std::endl;
                Logger::log_action("Sent token: " + session_key);
                boost::asio::write(socket, boost::asio::buffer(auth_response));
            }
            else {
                std::cout << "Invalid password received." << std::endl;
                Logger::log_action("Invalid password received");
                std::string error_response = "{\"status\":\"error\",\"message\":\"Invalid password\"}\n";
                boost::asio::write(socket, boost::asio::buffer(error_response));
                socket.close();
                return;
            }
        }
        else {
            std::cout << "No auth password provided." << std::endl;
            Logger::log_action("No auth password provided");
            std::string error_response = "{\"status\":\"error\",\"message\":\"Authentication required\"}\n";
            boost::asio::write(socket, boost::asio::buffer(error_response));
            socket.close();
            return;
        }

        if (is_authorized) {
            while (true) {
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, '\n');

                std::istream stream(&buffer);
                std::string message;
                std::getline(stream, message);
                std::cout << "Received message: " << message << std::endl;
                Logger::log_action("Received message: " + message);

                size_t token_pos = message.find("token:");
                if (token_pos != std::string::npos) {
                    std::string command = message.substr(0, token_pos);
                    command.erase(command.find_last_not_of(" \n\r\t") + 1);
                    command.erase(0, command.find_first_not_of(" \n\r\t"));
                    std::string received_token = message.substr(token_pos + 6);
                    received_token.erase(received_token.find_last_not_of(" \n\r\t") + 1);
                    received_token.erase(0, received_token.find_first_not_of(" \n\r\t"));

                    std::string expected_token = SecurityManager::generate_token(command, SecurityManager::get_session_key(&socket));
                    std::cout << "Expected token: " << expected_token << std::endl;
                    std::cout << "Received token: " << received_token << std::endl;
                    if (received_token == expected_token) {
                        if (!CpuMonitor::init_cpu_monitor()) {
                            std::cerr << "Failed to initialize CPU monitor!" << std::endl;
                            Logger::log_action("Failed to initialize CPU monitor");
                        }
                        json response = CommandHandler::handle_comand(command);
                        std::string response_str = response.dump() + "\n";
                        boost::asio::write(socket, boost::asio::buffer(response_str));
                        Logger::log_action("Sent response: " + response_str);
                    }
                    else {
                        std::cout << "Invalid token. Comparison failed." << std::endl;
                        Logger::log_action("Invalid token. Comparison failed");
                        std::string error_response = "{\"status\":\"error\",\"message\":\"Invalid token\"}\n";
                        boost::asio::write(socket, boost::asio::buffer(error_response));
                        socket.close();
                        SecurityManager::set_session_key(&socket, ""); 
                        return;
                    }
                }
                else {
                    std::cout << "No token in message." << std::endl;
                    Logger::log_action("No token in message");
                    std::string error_response = "{\"status\":\"error\",\"message\":\"Token required\"}\n";
                    boost::asio::write(socket, boost::asio::buffer(error_response));
                    socket.close();
                    SecurityManager::set_session_key(&socket, ""); 
                    return;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::log_action("Error: " + std::string(e.what()));
    }
}