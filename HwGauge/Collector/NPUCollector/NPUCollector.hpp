#pragma once

#ifdef HWGAUGE_USE_NPU

#include "Collector/DeviceCollector.hpp"
#include "NPUImpl.hpp"
#include "NPUDatabase.hpp"
#include "NPUCsvLogger.hpp"
#include "NPUPrometheus.hpp"

#include <iostream>

namespace hwgauge
{
    // 定义别名
    using NPUCollector = DeviceCollector<
        NPULabel, NPUMetrics, NPUImpl, NPUDatabase, NPUCsvLogger, NPUPrometheus
    >;
    
    // 定义特定的打印函数
    template<>
    inline void printMetric(const NPULabel& l, const NPUMetrics& m)
    {
        std::cout
            << "NPU{ "
            << "card="     << l.card_id
            << ", device=" << l.device_id
            << ", type="   << l.chip_type
            << ", name="   << l.chip_name

            << ", freqAICore=" << m.freq_aicore
            << ", freqAICPU="  << m.freq_aicpu
            << ", freqCtrlCPU="  << m.freq_ctrlcpu

            << ", utilAICore=" << m.util_aicore
            << ", utilAICPU="  << m.util_aicpu
            << ", utilCtrlCPU="  << m.util_ctrlcpu
            << ", utilVec="    << m.util_vec

            << ", memTotalMb=" <<m.mem_total_mb
            << ", memUsageMb="  <<m.mem_usage_mb
            << ", utilMem="      << m.util_mem
            << ", utilMemBW="    << m.util_membw
            << ", freqMem="  << m.freq_mem

            << ", chip_power="   << m.chip_power

            << ", temp="       << m.temperature
            << ", voltage="    << m.voltage
            << ", health="     << m.health
            << " }\n";
    }
}

#endif