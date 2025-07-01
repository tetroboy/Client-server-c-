#pragma once
#include "Libs.h"
#include "SecurityManager.h"
#include "Logger.h"
#include "Cpu_info.h"
#include "CommandHandler.h"

class ClientHandler {
public:
    static void handle_client(boost::asio::ip::tcp::socket socket);
};