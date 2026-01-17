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
        //利用率（%）
        int util_aicore; //AICore
        int util_aicpu; //AICPU
        int util_mem; //Mem
        int util_membw; //内存带宽
        int util_vec; //vector core
        //频率（MHz）
        int freq_aicore; //AICore
        int freq_aicpu; //AICPU
        int freq_mem; //Mem
        //功耗（W）
        double power;

        //其他
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

            << ", utilAICore=" << m.util_aicore
            << ", utilAICPU="  << m.util_aicpu
            << ", utilMem="    << m.util_mem
            << ", utilMemBW="  << m.util_membw
            << ", utilVec="    << m.util_vec

            << ", freqAICore=" << m.freq_aicore
            << ", freqAICPU="  << m.freq_aicpu
            << ", freqMem="    << m.freq_mem

            << ", power="      << m.power
            << ", temp="       << m.temperature
            << ", voltage="    << m.voltage
            << ", health="     << m.health
            << " }\n";
    }

}


#endif