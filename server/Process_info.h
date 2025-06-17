#pragma once
#include "libs.h"
json GetProcessListJson();
bool KillProcessByID(DWORD processID);
json start_process(const std::string& program_path);

