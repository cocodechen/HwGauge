#ifdef HWGAUGE_USE_INTEL_PCM

#include "CPUCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge
{

    std::string CPUCsvLogger::getHeader() const {
        return "Index,Name,Util(%),Freq(MHz),Temp(C),Power(W),"
               "C0(%),C6(%),MemRead(MB/s),MemWrite(MB/s),MemPower(W)";
    }

    std::string CPUCsvLogger::formatRow(const CPULabel& l, const CPUMetrics& m) const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2); // 统一设置浮点精度

        ss << l.index << ","
           << "\"" << l.name << "\","
           << m.cpuUtilization << ","
           << m.cpuFrequency << ","
           << m.temperature << ","
           << m.powerUsage << ","
           << m.c0Residency << ","
           << m.c6Residency << ","
           << m.memoryReadBandwidth << ","
           << m.memoryWriteBandwidth << ","
           << m.memoryPowerUsage;
        
        return ss.str();
    }
}
#endif