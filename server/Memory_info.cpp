#include "Memory_info.h"

json GetMemoryInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    json RAM_info = json::array();  
    if (GlobalMemoryStatusEx(&memInfo)) {
        
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
       
        DWORDLONG freePhysMem = memInfo.ullAvailPhys;
       
        DWORDLONG usedPhysMem = totalPhysMem - freePhysMem;

       
        double totalMB = totalPhysMem / (1024.0 * 1024.0);
        double usedMB = usedPhysMem / (1024.0 * 1024.0);
        double freeMB = freePhysMem / (1024.0 * 1024.0);

        double usagePercent = (usedPhysMem * 100.0) / totalPhysMem;
        json Ram = {
                    {"Total RAM:",totalMB, " MB"},
                    {"Used RAM:", usedMB, " MB"},
                    {"Free RAM:", freeMB, " MB"}
        };
        RAM_info.push_back(Ram);
        /*std::cout << "=== Memory Usage ===" << std::endl;
        std::cout << "Total RAM: " << std::fixed << std::setprecision(2) << totalMB << " MB" << std::endl;
        std::cout << "Used RAM: " << std::fixed << std::setprecision(2) << usedMB << " MB ("
            << std::setprecision(1) << usagePercent << "%)" << std::endl;
        std::cout << "Free RAM: " << std::fixed << std::setprecision(2) << freeMB << " MB" << std::endl;*/
    }
    else {
        std::cerr << "Failed to get memory info!" << std::endl;
    }
    return { {"RAM info", RAM_info} };
}

json get_disk_info_json() {
    json disk_info = json::array();  

    DWORD drives = GetLogicalDrives();
    char drive_letter = 'A';

    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            std::string root_path = std::string(1, drive_letter + i) + ":\\";
            ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;

            if (GetDiskFreeSpaceExA(root_path.c_str(), &free_bytes, &total_bytes, &total_free_bytes)) {
                json disk = {
                    {"drive", root_path},
                    {"total_gb", static_cast<double>(total_bytes.QuadPart) / (1024 * 1024 * 1024)},
                    {"free_gb", static_cast<double>(free_bytes.QuadPart) / (1024 * 1024 * 1024)},
                    {"used_gb", static_cast<double>(total_bytes.QuadPart - free_bytes.QuadPart) / (1024 * 1024 * 1024)}
                };
                disk_info.push_back(disk);
            }
        }
    }

    return { {"disks", disk_info} };  
}