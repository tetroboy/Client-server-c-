#pragma once
#include "Libs.h"

bool InitCpuMonitor();
std::vector<double> GetCoreUsages();
double GetTotalCpuUsage();