#include "Cpu_info.h"
#include "Memory_info.h"
#include "FileSystem.h"
#include "Process_info.h"
#include "NetConnection.h"
#include "Libs.h"
using boost::asio::ip::tcp;


std::unordered_set<std::string> blocked_ips;



void rotate_log_file();


std::ofstream log_file;
std::mutex log_mutex;
const size_t MAX_LOG_SIZE = 10 * 1024;

void log_action(const std::string& action, const std::string& client_key = "") {
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


void rotate_log_file() {
    std::string base_filename = "server_log.txt";
    int suffix = 1;
    std::string new_filename;

    while (true) {
        new_filename = base_filename + "-" + std::to_string(suffix);
        if (!std::filesystem::exists(new_filename)) {
            break;
        }
        ++suffix;
    }

    if (log_file.is_open()) {
        log_file.close();
    }
    std::filesystem::rename("server_log.txt", new_filename);
    log_file.open("server_log.txt", std::ios::app);
    if (!log_file.is_open()) {
        std::cout << "Failed to create new log file after rotation!" << std::endl;
    }
}


json handle_comand(const std::string &s);

using boost::asio::ip::tcp;
using json = nlohmann::json;

std::string generateRandomPassword(int length = 16) {
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

std::string sessionPassword = generateRandomPassword();


std::string generateToken(const std::string& data, const std::string& key) {
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), NULL, NULL);
    char mdString[65];
    for (int i = 0; i < 32; i++) {
        sprintf_s(&mdString[i * 2], 3, "%02x", (unsigned int)digest[i]);
    }
    return std::string(mdString);
}

std::unordered_map<tcp::socket*, std::string> sessionKeys;

void handle_client(tcp::socket socket) {
    try {
        bool isAuthorized = false;
        std::string sessionKey;

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');

        std::istream stream(&buffer);
        std::string message;
        std::getline(stream, message);
        std::cout << "Received initial message: " << message << std::endl;

        if (message.find("auth ") == 0) {
            std::string receivedPassword = message.substr(5);
            extern std::string sessionPassword;
            if (receivedPassword == sessionPassword) {
                isAuthorized = true;

                std::cout << "Client authorized successfully." << std::endl;
                log_action("Client authorized successfully");

                sessionKey = generateRandomPassword(32);
                sessionKeys[&socket] = sessionKey;
                std::string authResponse = "{\"status\":\"success\",\"message\":\"Authorized\",\"token\":\"" + sessionKey + "\"}\n";

                std::cout << "Sent token: " << sessionKey << std::endl;
                log_action("Sent token: " + sessionKey);

                boost::asio::write(socket, boost::asio::buffer(authResponse));
            }
            else {
                std::cout << "Invalid password received." << std::endl;
                log_action("Invalid password received");

                std::string errorResponse = "{\"status\":\"error\",\"message\":\"Invalid password\"}\n";
                boost::asio::write(socket, boost::asio::buffer(errorResponse));
                socket.close();
                return;
            }
        }
        else {
            std::cout << "No auth password provided." << std::endl;
            log_action("No auth password provided");

            std::string errorResponse = "{\"status\":\"error\",\"message\":\"Authentication required\"}\n";
            boost::asio::write(socket, boost::asio::buffer(errorResponse));
            socket.close();
            return;
        }

        if (isAuthorized) {
            while (true) {
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, '\n');

                std::istream stream(&buffer);
                std::string message;
                std::getline(stream, message);
                std::cout << "Received message: " << message << std::endl;
                log_action("Received message: " + message);

                size_t tokenPos = message.find("token:");
                if (tokenPos != std::string::npos) {
                    std::string command = message.substr(0, tokenPos);
                    // 
                    command.erase(command.find_last_not_of(" \n\r\t") + 1);
                    command.erase(0, command.find_first_not_of(" \n\r\t"));
                    std::string receivedToken = message.substr(tokenPos + 6);
                    receivedToken.erase(receivedToken.find_last_not_of(" \n\r\t") + 1);
                    receivedToken.erase(0, receivedToken.find_first_not_of(" \n\r\t"));

                    auto it = sessionKeys.find(&socket);
                    if (it != sessionKeys.end()) {
                        std::string expectedToken = generateToken(command, it->second);
                        std::cout << "Expected token: " << expectedToken << std::endl;
                        std::cout << "Received token: " << receivedToken << std::endl;
                        if (receivedToken == expectedToken) {
                            if (!InitCpuMonitor()) {
                                std::cerr << "Failed to initialize CPU monitor!" << std::endl;
                                log_action("Failed to initialize CPU monitor");
                            }
                            json response = handle_comand(command);
                            std::string response_str = response.dump() + "\n";
                            boost::asio::write(socket, boost::asio::buffer(response_str));
                            log_action("Sent response: " + response_str);
                        }
                        else {
                            std::cout << "Invalid token. Comparison failed." << std::endl;
                            log_action("Invalid token. Comparison failed");

                            std::string errorResponse = "{\"status\":\"error\",\"message\":\"Invalid token\"}\n";
                            boost::asio::write(socket, boost::asio::buffer(errorResponse));
                            socket.close();
                            sessionKeys.erase(&socket);
                            return;
                        }
                    }
                    else {
                        std::cout << "Session key not found." << std::endl;
                        log_action("Session key not found");

                        socket.close();
                        return;
                    }
                }
                else {
                    std::cout << "No token in message." << std::endl;
                    log_action("No token in message");

                    std::string errorResponse = "{\"status\":\"error\",\"message\":\"Token required\"}\n";
                    boost::asio::write(socket, boost::asio::buffer(errorResponse));
                    socket.close();
                    sessionKeys.erase(&socket);
                    return;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        log_action("Error: " + std::string(e.what()));
    }
}

json handle_comand(const std::string& s) {
    if (s == "CPU usage") {
        return GetCpuUsageJson();
    }
    if (s == "RAM usage") {
        return GetMemoryInfo();
    }
    if (s == "Disk usage") {
        return get_disk_info_json();
    }
    if (s == "PID list") {
        return GetProcessListJson();
    }
    if (s.find("kill_process") == 0) {
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            try {
                DWORD pid = std::stoul(s.substr(pos + 1));
                if (KillProcessByID(pid)) {
                    return { {"status", "success"}, {"message", "Process " + std::to_string(pid) + " killed"} };
                }
                else {
                    return { {"status", "error"}, {"message", "Failed to kill process " + std::to_string(pid)} };
                }
            }
            catch (...) {
                return { {"status", "error"}, {"message", "Invalid PID format"} };
            }
        }
        else {
            return { {"status", "error"}, {"message", "Usage: kill_process <PID>"} };
        }
    }
    if (s.find("Read_file") == 0) {
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string path = s.substr(pos + 1);
            std::cout << "path input " << path;
            return read_file(path);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: Read_file <path>"} };
        }
    }
    if (s.find("Show_files") == 0) {
        std::cout << "i`m here";
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string path = s.substr(pos + 1);
            std::cout << "path input " << path;
            return get_directory_contents(path);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: Read_file <path>"} };
        }
    }
    if (s.find("Delete_file") == 0) {
        std::cout << "i`m here";
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string path = s.substr(pos + 1);
            std::cout << "path input " << path;
            return delete_path(path);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: Read_file <path>"} };
        }
    }
    if (s == "SHOW_PORTS") {
        return get_open_ports();
    }
    if (s.find("BLOCK_IP") == 0) { //TODO
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string address = s.substr(pos + 1);
            return block_ip(address);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: BLOCK_IP <address>"} };
        }
    }
    if (s.find("PING") == 0) { //todo logic not work
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string address = s.substr(pos + 1);
            return ping_address(address);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: PING <address>"} };
        }
    }
    if (s.find("START_PROCESS") == 0) {
        size_t pos = s.find_first_of(' ');
        if (pos != std::string::npos) {
            std::string program_path = s.substr(pos + 1);
            size_t tokenPos = program_path.find(" token:");
            if (tokenPos != std::string::npos) {
                program_path = program_path.substr(0, tokenPos);
            }
            std::cout << "Opening file: " << program_path << std::endl;
            json response = start_process(program_path);
            std::cout << "Process response: " << response.dump() << std::endl;
            return response;
        }
        else {
            return { {"status", "error"}, {"message", "Usage: START_PROCESS <program_path>"} };
        }
    }
    return { {"status", "error"}, {"message", "Unknown command"} };
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 1234));

        log_file.open("server_log.txt", std::ios::app);
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
                return 1;
            }
        }
        log_action("Server started. Session password: " + sessionPassword);
        std::cout << "Server started. Session password: " << sessionPassword << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "Connected new user" << std::endl;
            log_action("Connected new user");
            std::thread(handle_client, std::move(socket)).detach();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        log_action("Server error: " + std::string(e.what()));
    }

    if (log_file.is_open()) {
        log_file.close();
    }

    return 0;
}