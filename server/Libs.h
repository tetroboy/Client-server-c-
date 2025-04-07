#pragma once
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <iostream>
#include "TCHAR.h"
#include <windows.h>
#include <pdh.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <winternl.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "ntdll.lib")

using json = nlohmann::json;