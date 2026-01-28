#pragma once

#ifdef HWGAUGE_USE_INTEL_PCM

#include "Collector/Base/DeviceCollector.hpp"
#include "PCM.hpp"
#include "CPUDatabase.hpp"
#include "CPUCsvLogger.hpp"
#include "CPUPrometheus.hpp"

#include <iostream>

namespace hwgauge
{
    // 定义别名
    using CPUCollector = DeviceCollector<
        CPULabel, CPUMetrics, PCM, CPUDatabase, CPUCsvLogger, CPUPrometheus
    >;
    
    // 定义特定的打印函数
    template<>
    inline void printMetric(const CPULabel& l, const CPUMetrics& m)
    {
        std::cout
            << "CPU{ "
            << "index="     << l.index
            << ", name=" << l.name
            << ", temp=" << m.temperature << "C"
            << ", cpuUtilization="   << m.cpuUtilization
            << ", cpuFrequency="   << m.cpuFrequency
            << ", c0Residency=" << m.c0Residency
            << ", c6Residency="  << m.c6Residency
            << ", powerUsage="    << m.powerUsage
            << ", memoryReadBandwidth="  << m.memoryReadBandwidth
            << ", memoryWriteBandwidth="    << m.memoryWriteBandwidth
            << ", memoryPowerUsage=" << m.memoryPowerUsage
            << " }\n";
    }
}

#endif