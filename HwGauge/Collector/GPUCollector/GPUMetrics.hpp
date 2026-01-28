#pragma once

#ifdef HWGAUGE_USE_NVML

#include <string>

namespace hwgauge
{
    struct GPULabel
    {
        std::size_t index;
        std::string name;
    };

    struct GPUMetrics
    {
        double gpuUtilization;
        double memoryUtilization;
        double gpuFrequency;
        double memoryFrequency;
        double powerUsage;

        double temperature;
    };
}

#endif
