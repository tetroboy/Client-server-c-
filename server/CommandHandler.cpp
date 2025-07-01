#include "CommandHandler.h"


json CommandHandler::handle_comand(const std::string& s) {
    if (s == "CPU usage") {
        return CpuMonitor::get_cpu_usage_json();
    }
    if (s == "RAM usage") {
        return MemoryInfoManager::get_memory_info();
    }
    if (s == "Disk usage") {
        return MemoryInfoManager::get_disk_info();
    }
    if (s == "PID list") {
        return ProcessManager::get_process_list();
    }
    if (s.find("kill_process") == 0) {
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            try {
                DWORD pid = std::stoul(s.substr(pos + 1));
                if (ProcessManager::kill_process_by_id(pid)) {
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
            return FileSystemManager::read_file(path);
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
            return FileSystemManager::get_directory_contents(path);
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
            return FileSystemManager::delete_path(path);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: Read_file <path>"} };
        }
    }
    if (s == "SHOW_PORTS") {
        return get_open_ports();
    }
    if (s.find("BLOCK_IP") == 0) {
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            std::string address = s.substr(pos + 1);
            return block_ip(address);
        }
        else {
            return { {"status", "error"}, {"message", "Usage: BLOCK_IP <address>"} };
        }
    }
    if (s.find("PING") == 0) {
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
            size_t token_pos = program_path.find(" token:");
            if (token_pos != std::string::npos) {
                program_path = program_path.substr(0, token_pos);
            }
            std::cout << "Opening file: " << program_path << std::endl;
            json response = ProcessManager::start_process(program_path);
            std::cout << "Process response: " << response.dump() << std::endl;
            return response;
        }
        else {
            return { {"status", "error"}, {"message", "Usage: START_PROCESS <program_path>"} };
        }
    }
    return { {"status", "error"}, {"message", "Unknown command"} };
}