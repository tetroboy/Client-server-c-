#pragma once
#include "Libs.h"

json get_open_ports();
json block_ip(const std::string& address);
json ping_address(const std::string& address);

extern std::unordered_set<std::string> blocked_ips;