#include "Memory_info.h"
json MemoryInfoManager::get_memory_info() {
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(mem_info);
    json ram_info = json::array();

    if (GlobalMemoryStatusEx(&mem_info)) {
        ULARGE_INTEGER total_phys_mem = { mem_info.ullTotalPhys };
        ULARGE_INTEGER free_phys_mem = { mem_info.ullAvailPhys };
        ULARGE_INTEGER used_phys_mem = { total_phys_mem.QuadPart - free_phys_mem.QuadPart };

        double total_mb = MemoryInfoManager::bytes_to_gb(total_phys_mem) * 1024;
        double used_mb = MemoryInfoManager::bytes_to_gb(used_phys_mem) * 1024;
        double free_mb = MemoryInfoManager::bytes_to_gb(free_phys_mem) * 1024;

        double usage_percent = (static_cast<double>(used_phys_mem.QuadPart) * 100.0) / total_phys_mem.QuadPart;

        json ram = {
            {"total_ram", total_mb},
            {"used_ram", used_mb},
            {"free_ram", free_mb},
            {"usage_percent", usage_percent}
        };
        ram_info.push_back(ram);
    }
    else {
        std::cerr << "Failed to get memory info!" << std::endl;
    }
    return { {"ram_info", ram_info} };
}

json MemoryInfoManager::get_disk_info() {
    json disk_info = json::array();
    const int MAX_DRIVE_COUNT = 26; 

    DWORD drives = GetLogicalDrives();
    char drive_letter = 'A';

    for (int i = 0; i < MAX_DRIVE_COUNT; ++i) {
        if (drives & (1 << i)) {
            std::string root_path = std::string(1, drive_letter + i) + ":\\";
            ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;

            if (GetDiskFreeSpaceExA(root_path.c_str(), &free_bytes, &total_bytes, &total_free_bytes)) {
                json disk = {
                    {"drive", root_path},
                    {"total_gb", MemoryInfoManager::bytes_to_gb(total_bytes)},
                    {"free_gb", MemoryInfoManager::bytes_to_gb(free_bytes)},
                    {"used_gb", MemoryInfoManager::bytes_to_gb(total_bytes) - MemoryInfoManager::bytes_to_gb(free_bytes)}
                };
                disk_info.push_back(disk);
            }
        }
    }

    return { {"disks", disk_info} };
}