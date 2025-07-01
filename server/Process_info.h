#pragma once
#include "libs.h"

class ProcessManager {
public:
    static json get_process_list();
    static json start_process(const std::string& program_path);
    static bool kill_process_by_id(DWORD process_id);
};