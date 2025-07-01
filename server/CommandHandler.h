#pragma once
#include "Libs.h"
#include "Cpu_info.h"
#include "Memory_info.h"
#include "FileSystem.h"
#include "Process_info.h"
#include "NetConnection.h"

class CommandHandler
{
public:
	static json handle_comand(const std::string& s);
};

