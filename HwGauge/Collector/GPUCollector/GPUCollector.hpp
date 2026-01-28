#pragma once

#ifdef HWGAUGE_USE_NVML

#include "Collector/Base/DeviceCollector.hpp"
#include "NVML.hpp"
#include "GPUDatabase.hpp"
#include "GPUCsvLogger.hpp"
#include "GPUPrometheus.hpp"

#include <iostream>

namespace hwgauge
{
    // 定义别名
    using GPUCollector = DeviceCollector<
        GPULabel, GPUMetrics, NVML, GPUDatabase, GPUCsvLogger, GPUPrometheus
    >;
    
    // 定义特定的打印函数
    template<>
    inline void printMetric(const GPULabel& l, const GPUMetrics& m)
    {
        std::cout
            << "GPU{ "
            << "index="    << l.index
            << ", name="   << l.name
            << ", util="   << m.gpuUtilization
            << ", memUtil="<< m.memoryUtilization
            << ", gpuFreq="<< m.gpuFrequency
            << ", memFreq="<< m.memoryFrequency
            << ", power="  << m.powerUsage
            << ", temp="   << m.temperature<<"C"
            << " }\n";
    }
}

#endif