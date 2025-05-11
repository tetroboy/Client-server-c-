#include "Cpu_info.h"

static std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> prevCoreInfo;
static ULONGLONG prevTotalIdle = 0, prevTotalKernel = 0, prevTotalUser = 0;
static int coreCount = 0;


bool InitCpuMonitor() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    coreCount = sysInfo.dwNumberOfProcessors;
    prevCoreInfo.resize(coreCount);

    ULONG bufferSize = coreCount * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    NTSTATUS status = NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        prevCoreInfo.data(),
        bufferSize,
        nullptr
    );

    
    for (const auto& core : prevCoreInfo) {
        prevTotalIdle += core.IdleTime.QuadPart;
        prevTotalKernel += core.KernelTime.QuadPart;
        prevTotalUser += core.UserTime.QuadPart;
    }

    return NT_SUCCESS(status);
}


std::vector<double> GetCoreUsages() {
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> currentInfo(coreCount);
    ULONG bufferSize = coreCount * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        currentInfo.data(),
        bufferSize,
        nullptr
    );

    std::vector<double> usages;
    for (int i = 0; i < coreCount; i++) {
        ULONGLONG idleDiff = currentInfo[i].IdleTime.QuadPart - prevCoreInfo[i].IdleTime.QuadPart;
        ULONGLONG kernelDiff = currentInfo[i].KernelTime.QuadPart - prevCoreInfo[i].KernelTime.QuadPart;
        ULONGLONG userDiff = currentInfo[i].UserTime.QuadPart - prevCoreInfo[i].UserTime.QuadPart;

        ULONGLONG total = kernelDiff + userDiff;
        double usage = (total > 0) ? (100.0 - (100.0 * idleDiff) / total) : 0.0;
        usages.push_back(std::min(100.0, std::max(0.0, usage)));
    }

    prevCoreInfo = currentInfo;
    return usages;
}


double GetTotalCpuUsage() {
    ULONGLONG totalIdle = 0, totalKernel = 0, totalUser = 0;

    for (const auto& core : prevCoreInfo) {
        totalIdle += core.IdleTime.QuadPart;
        totalKernel += core.KernelTime.QuadPart;
        totalUser += core.UserTime.QuadPart;
    }

    ULONGLONG idleDiff = totalIdle - prevTotalIdle;
    ULONGLONG kernelDiff = totalKernel - prevTotalKernel;
    ULONGLONG userDiff = totalUser - prevTotalUser;

    prevTotalIdle = totalIdle;
    prevTotalKernel = totalKernel;
    prevTotalUser = totalUser;

    ULONGLONG total = kernelDiff + userDiff;
    return (total > 0) ? (100.0 - (100.0 * idleDiff) / total) : 0.0;
}

json GetCpuUsageJson() {
    std::vector<double> coreUsages;
    double totalUsage;

    
    for (int i = 0; i < 2; ++i) {
        Sleep(100);
        coreUsages = GetCoreUsages();
        totalUsage = GetTotalCpuUsage();
    }

    
    json cpuInfo;
    json coresArray = json::array();
    for (size_t i = 0; i < coreUsages.size(); i++) {
        json coreInfo = {
            {"core_id", i},
            {"usage_percent", std::round(coreUsages[i] * 10) / 10} 
        };
        coresArray.push_back(coreInfo);
    }


    cpuInfo["cores"] = coresArray;
    cpuInfo["total_usage"] = std::round(totalUsage * 10) / 10;
    cpuInfo["core_count"] = coreUsages.size();

    return cpuInfo;
}