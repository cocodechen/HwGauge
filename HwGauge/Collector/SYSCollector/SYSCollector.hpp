#pragma once

#ifdef __linux__

#include "Collector/DeviceCollector.hpp"
#include "SYSImpl.hpp"
#include "SYSDatabase.hpp"
#include "SYSCsvLogger.hpp"
#include "SYSPrometheus.hpp"

#include <iostream>

namespace hwgauge
{
    // 定义别名
    using SYSCollector = DeviceCollector<
        SYSLabel, SYSMetrics, SYSImpl, SYSDatabase, SYSCsvLogger, SYSPrometheus
    >;
    
    // 定义特定的打印函数
    template<>
    inline void printMetric(const SYSLabel& l, const SYSMetrics& m)
    {
        std::cout << "SYS{ "
            << "Machine: "<<l.name<<", "
            << "Mem: " << m.memUsedGB << "/" << m.memTotalGB << "GB (" << m.memUtilizationPercent << "%), "
            << "Disk: R=" << m.diskReadMBps << " W=" << m.diskWriteMBps << " MB/s (MaxUtil: " << m.maxDiskUtilPercent << "%), "
            << "Net: In=" << m.netDownloadMBps << " Out=" << m.netUploadMBps << " MB/s, "
            << "Power: " << m.systemPowerWatts << " W"
            << " }\n";
    }
}

#endif