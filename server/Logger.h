#pragma once
#include "Libs.h"

class Logger {
public:
    static void log_action(const std::string& action, const std::string& client_key = "");
    static void rotate_log_file();

private:
    static std::ofstream log_file;
    static std::mutex log_mutex;
    static const size_t MAX_LOG_SIZE;
};

