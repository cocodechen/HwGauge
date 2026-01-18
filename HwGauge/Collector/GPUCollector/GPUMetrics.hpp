#pragma once

#ifdef HWGAUGE_USE_NVML

#include <string>
#include <iostream>

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
    };

    //#pragma once 只防止单个编译单元内部重复展开，不防止多个译单元各自展开
    //因此在头文件实现函数，定义为inline
    inline void outGPU(const GPULabel& l, const GPUMetrics& m)
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
            << " }\n";
    }
}

#endif
