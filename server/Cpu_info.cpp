#include "Cpu_info.h"


std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> CpuMonitor::prev_core_info;
ULONGLONG CpuMonitor::prev_total_idle = 0;
ULONGLONG CpuMonitor::prev_total_kernel = 0;
ULONGLONG CpuMonitor::prev_total_user = 0;
int CpuMonitor::core_count = 0;

bool CpuMonitor::init_cpu_monitor() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    core_count = sys_info.dwNumberOfProcessors;
    prev_core_info.resize(core_count);

    ULONG buffer_size = core_count * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    NTSTATUS status = NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        prev_core_info.data(),
        buffer_size,
        nullptr
    );

    for (const auto& core : prev_core_info) {
        prev_total_idle += core.IdleTime.QuadPart;
        prev_total_kernel += core.KernelTime.QuadPart;
        prev_total_user += core.UserTime.QuadPart;
    }

    return NT_SUCCESS(status);
}

std::vector<double> CpuMonitor::get_core_usages() {
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> current_info(core_count);
    ULONG buffer_size = core_count * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        current_info.data(),
        buffer_size,
        nullptr
    );

    std::vector<double> usages;
    for (int i = 0; i < core_count; i++) {
        ULONGLONG idle_diff = current_info[i].IdleTime.QuadPart - prev_core_info[i].IdleTime.QuadPart;
        ULONGLONG kernel_diff = current_info[i].KernelTime.QuadPart - prev_core_info[i].KernelTime.QuadPart;
        ULONGLONG user_diff = current_info[i].UserTime.QuadPart - prev_core_info[i].UserTime.QuadPart;

        ULONGLONG total = kernel_diff + user_diff;
        double usage = (total > 0) ? (100.0 - (100.0 * idle_diff) / total) : 0.0;
        usages.push_back(std::min(100.0, std::max(0.0, usage)));
    }

    prev_core_info = current_info;
    return usages;
}

double CpuMonitor::get_total_cpu_usage() {
    ULONGLONG total_idle = 0, total_kernel = 0, total_user = 0;

    for (const auto& core : prev_core_info) {
        total_idle += core.IdleTime.QuadPart;
        total_kernel += core.KernelTime.QuadPart;
        total_user += core.UserTime.QuadPart;
    }

    ULONGLONG idle_diff = total_idle - prev_total_idle;
    ULONGLONG kernel_diff = total_kernel - prev_total_kernel;
    ULONGLONG user_diff = total_user - prev_total_user;

    prev_total_idle = total_idle;
    prev_total_kernel = total_kernel;
    prev_total_user = total_user;

    ULONGLONG total = kernel_diff + user_diff;
    return (total > 0) ? (100.0 - (100.0 * idle_diff) / total) : 0.0;
}

json CpuMonitor::get_cpu_usage_json() {
    std::vector<double> core_usages;
    double total_usage;

    for (int i = 0; i < 2; ++i) {
        Sleep(100);
        core_usages = get_core_usages();
        total_usage = get_total_cpu_usage();
    }

    json cpu_info;
    json cores_array = json::array();
    for (size_t i = 0; i < core_usages.size(); i++) {
        json core_info = {
            {"core_id", i},
            {"usage_percent", std::round(core_usages[i] * 10) / 10}
        };
        cores_array.push_back(core_info);
    }

    cpu_info["cores"] = cores_array;
    cpu_info["total_usage"] = std::round(total_usage * 10) / 10;
    cpu_info["core_count"] = core_usages.size();

    return cpu_info;
}