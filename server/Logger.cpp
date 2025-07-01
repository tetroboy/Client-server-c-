#include "Logger.h"
#include <filesystem>
#include <ctime>
#include <sstream>
#include <iostream>

std::ofstream Logger::log_file;
std::mutex Logger::log_mutex;
const size_t Logger::MAX_LOG_SIZE = 10 * 1024;

void Logger::log_action(const std::string& action, const std::string& client_key) {
    std::lock_guard<std::mutex> lock(log_mutex);

    if (log_file.is_open()) {
        log_file.seekp(0, std::ios::end);
        if (log_file.tellp() >= MAX_LOG_SIZE) {
            log_file.close();
            rotate_log_file();
        }
    }

    if (!log_file.is_open()) {
        std::string log_filename = "server_log.txt";
        if (!std::filesystem::exists(log_filename)) {
            auto now = std::time(nullptr);
            std::tm tm;
            localtime_s(&tm, &now);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y%m%d");
            log_filename = "server_log_" + oss.str() + ".txt";
        }
        log_file.open(log_filename, std::ios::app);
        if (!log_file.is_open()) {
            std::cout << "Failed to open log file: " << log_filename << std::endl;
            return;
        }
    }

    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    char timestamp[26];
    ctime_s(timestamp, sizeof(timestamp), &now);
    timestamp[24] = '\0';
    std::string log_entry = timestamp + std::string(" - ") + (client_key.empty() ? "" : "[" + client_key.substr(0, 8) + "] ") + action;
    log_file << log_entry << std::endl;
    log_file.flush();
}

void Logger::rotate_log_file() {
    std::string base_name = "server_log"; 
    std::string extension = ".txt";      
    std::string full_filename = base_name + extension; 
    int suffix = 1;
    std::string new_filename;

    while (true) {
        new_filename = base_name + "-" + std::to_string(suffix) + extension;
        if (!std::filesystem::exists(new_filename)) {
            break;
        }
        ++suffix;
    }

    if (log_file.is_open()) {
        log_file.close();
    }
    std::filesystem::rename(full_filename, new_filename);
    log_file.open(full_filename, std::ios::app);
    if (!log_file.is_open()) {
        std::cout << "Failed to create new log file after rotation!" << std::endl;
    }
}