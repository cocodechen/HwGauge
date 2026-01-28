#ifdef HWGAUGE_USE_NVML

#include "GPUCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge
{
    std::string GPUCsvLogger::getHeader()const
    {
        return "Index,Name,GpuUtil(%),MemUtil(%),GpuFreq(MHz),MemFreq(MHz),Power(W),Temp(C)";
    }

    std::string GPUCsvLogger::formatRow(const GPULabel& l, const GPUMetrics& m) const
    {
        std::stringstream ss;
        ss << l.index << ","
           << "\"" << l.name << "\","  // 处理名称中可能含有的特殊字符
           << std::fixed << std::setprecision(2) << m.gpuUtilization << ","
           << std::fixed << std::setprecision(2) << m.memoryUtilization << ","
           << static_cast<int>(m.gpuFrequency) << ","  // 频率通常看整数即可
           << static_cast<int>(m.memoryFrequency) << ","
           << std::fixed << std::setprecision(2) << m.powerUsage << ","
           << std::fixed << std::setprecision(1) << m.temperature;
        return ss.str();
    }

}

#endif