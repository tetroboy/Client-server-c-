#pragma once
#include "Libs.h"

json get_directory_contents(const std::string& dir_path_str);
json delete_path(const std::string& path_str, bool allow_directory_deletion = false);
json read_file(const std::string& path_str);