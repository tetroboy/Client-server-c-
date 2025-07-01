#pragma once
#include "Libs.h" 

class CpuMonitor {
public:
    
    static bool init_cpu_monitor();
    static std::vector<double> get_core_usages();
    static double get_total_cpu_usage();
    static json get_cpu_usage_json();

private:
    
    static std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> prev_core_info;
    static ULONGLONG prev_total_idle;
    static ULONGLONG prev_total_kernel;
    static ULONGLONG prev_total_user;
    static int core_count;
};