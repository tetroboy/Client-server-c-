#include "FileSystem.h"
#include <Windows.h>
#include <fstream>

json get_directory_contents(const std::string& dir_path_str) {
    json result;
    fs::path dir_path(dir_path_str);

    
    auto to_utf8 = [](const std::wstring& wstr) -> std::string {
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "";
        std::string utf8_str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], size, nullptr, nullptr);
        utf8_str.pop_back(); 
        return utf8_str;
    };


    auto get_utf8_path = [&to_utf8](const fs::path& p) -> std::string {
        try {
            return to_utf8(p.wstring());
        }
        catch (...) {
            try {
                return p.string();
            }
            catch (...) {
                return "[invalid_name]";
            }
        }
    };

    result["path"] = get_utf8_path(dir_path);
    result["directories"] = json::array();
    result["files"] = json::array();

    try {
        if (!fs::exists(dir_path)) {
            result["error"] = "Directory does not exist";
            return result;
        }

        if (!fs::is_directory(dir_path)) {
            result["error"] = "Path is not a directory";
            return result;
        }

        for (const auto& entry : fs::directory_iterator(dir_path)) {
            try {
                if (entry.is_directory()) {
                    json dir_info;
                    dir_info["name"] = get_utf8_path(entry.path().filename());
                    dir_info["path"] = get_utf8_path(entry.path());
                    result["directories"].push_back(dir_info);
                }
                else if (entry.is_regular_file()) {
                    json file_info;
                    file_info["name"] = get_utf8_path(entry.path().filename());
                    file_info["path"] = get_utf8_path(entry.path());
                    file_info["size"] = entry.file_size();
                    file_info["extension"] = get_utf8_path(entry.path().extension());
                    result["files"].push_back(file_info);
                }
            }
            catch (...) {
                continue; 
            }
        }
    }
    catch (const std::exception& e) {
        result["error"] = e.what();
    }

    return result;
}

json delete_path(const std::string& path_str, bool allow_directory_deletion ) {
    json result;
    fs::path path_to_delete(path_str);

    try {

        if (!fs::exists(path_to_delete)) {
            result["status"] = "error";
            result["message"] = "Path does not exist";
            return result;
        }


        if (fs::is_regular_file(path_to_delete)) {
            if (fs::remove(path_to_delete)) {
                result["status"] = "success";
                result["message"] = "File deleted successfully";
                result["type"] = "file";
            }
            else {
                result["status"] = "error";
                result["message"] = "Failed to delete file";
            }
        }

        else if (fs::is_directory(path_to_delete)) {
            if (!allow_directory_deletion) {
                result["status"] = "error";
                result["message"] = "Directory deletion not allowed (use allow_directory_deletion=true)";
                return result;
            }


            uintmax_t deleted_count = fs::remove_all(path_to_delete);
            result["status"] = "success";
            result["message"] = "Directory and its contents deleted successfully";
            result["deleted_items_count"] = deleted_count;
            result["type"] = "directory";
        }
        else {
            result["status"] = "error";
            result["message"] = "Path is neither a file nor a directory";
        }
    }
    catch (const fs::filesystem_error& e) {
        result["status"] = "error";
        result["message"] = e.what();
    }
    catch (const std::exception& e) {
        result["status"] = "error";
        result["message"] = std::string("Unexpected error: ") + e.what();
    }

    return result;
}

json read_file(const std::string& path_str) {
    json result;
    fs::path path_to_read(path_str);
    std::cout << "\n" << path_str << "\n";
    try {
        if (fs::is_regular_file(path_to_read)) {
            std::ifstream file(path_to_read);
            if (!file.is_open()) {
                std::cerr << "������ �������� �����!" << std::endl;
                return 1;
            }

            std::string content;
            std::string line;
            while (std::getline(file, line)) {
                content += line + "\n"; 
            }
            if (file.bad()) {
                result["status"] = "error";
                result["message"] = "������ ������ �����!";
                return result;
            }
            file.close();
            result["status"] = "success";
            result["file_content"] = content;
            
            
            
        }
        else {
            result["status"] = "error";
            result["message"] = "Path is neither a file nor a directory";
        }
    }
    catch (const fs::filesystem_error& e) {
        result["status"] = "error";
        result["message"] = "File error: " + std::string(e.what());
    }
    catch (const std::exception& e) {
        result["status"] = "error";
        result["message"] = "Error: " + std::string(e.what());
    }
    catch (...) {
        result["status"] = "error";
        result["message"] = "Unknown Error!";
    }
    return result;
    std::cout << "\n" << "END" << "\n";
}