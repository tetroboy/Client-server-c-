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
#include <nlohmann/json.hpp>
#include <filesystem>
#include <unordered_set>
#include <shellapi.h>
#include <string>
#include <unordered_map>
#include <random>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <fstream>
#include <mutex>
#include <ctime>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "ntdll.lib")
//#define _WIN32_WINNT 0x0A00  // Windows 10
using json = nlohmann::json;
namespace fs = std::filesystem;

