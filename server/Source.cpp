#include "Cpu_info.h"
#include "Memory_info.h"
#include "FileSystem.h"
#include "Process_info.h"
#include "NetConnection.h"
#include "Libs.h"
#include "Logger.h"
#include "SecurityManager.h"
#include "ClientHandler.h"
#include "CommandHandler.h"
using boost::asio::ip::tcp;

std::unordered_set<std::string> blocked_ips;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 1234));

        Logger::log_action("Server started. Session password: " + SecurityManager::get_session_password());
        std::cout << "Server started. Session password: " << SecurityManager::get_session_password() << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "Connected new user" << std::endl;
            Logger::log_action("Connected new user");
            std::thread(ClientHandler::handle_client, std::move(socket)).detach();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        Logger::log_action("Server error: " + std::string(e.what()));
    }

    return 0;
}