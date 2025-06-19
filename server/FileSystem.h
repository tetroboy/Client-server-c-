#pragma once
#include "Libs.h"
#include "FileSystem.h"
#include <Windows.h>
#include <fstream>

class FileSystemManager {
private:
   
    static std::string to_utf8(const std::wstring& wstr);

    
    static std::string get_utf8_path(const fs::path& p);

public:
    
    static json get_directory_contents(const std::string& dir_path_str);
    static json delete_path(const std::string& path_str, bool allow_directory_deletion = false);
    static json read_file(const std::string& path_str);
};