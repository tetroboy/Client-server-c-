#pragma once
#include "libs.h"

class MemoryInfoManager {
private:
    inline static double bytes_to_gb(ULARGE_INTEGER bytes) {
        return static_cast<double>(bytes.QuadPart) / (1024 * 1024 * 1024);
    }

public:
    static json get_memory_info();

    static json get_disk_info();
};

