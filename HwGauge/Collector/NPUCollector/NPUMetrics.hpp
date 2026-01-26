#pragma once

#ifdef HWGAUGE_USE_NPU

#include <string>
#include <iostream>

namespace hwgauge
{
    struct NPULabel
    {
        int card_id;
        int device_id;
        std::string chip_type; //芯片类型
        std::string chip_name; //芯片名称
    };

    struct NPUMetrics
    {
        // --- 频率 ---
        int freq_aicore; // AICore 频率 (MHz)
        int freq_aicpu;  // AICPU 频率 (MHz)
        int freq_ctrlcpu;// CtrlCPU 频率 (MHz)

        // --- 算力负载 ---
        int util_aicore; // AICore 利用率 (%)
        int util_aicpu;  // AICPU 利用率 (%)
        int util_ctrlcpu;// CtrlCPU 利用率 (%)
        int util_vec;    // Vector Core 利用率 (%)

        // --- 存储资源 为片上内存---
        long long mem_total_mb; // 总显存 (MB)
        long long mem_usage_mb;  // 已用显存 (MB)
        int util_mem;    // 显存已用百分比 (%) -> 由 (Total-Free)/Total 计算得出
        int util_membw;  // 显存带宽利用率 (%) 
        int freq_mem;    // 显存频率 (MHz)

        // --- 功耗（W）---
        double chip_power;

        // --- 环境 ---
        //健康状态 (0: OK, 1: WARN, 2: ERROR, 3: CRITICAL, 0xFFFFFFFF: NOT_EXIST)
        unsigned int health;
        //温度（C）
        int temperature;
        //电压（V）
        double voltage;
    };

    inline void outNPU(const NPULabel& l, const NPUMetrics& m)
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