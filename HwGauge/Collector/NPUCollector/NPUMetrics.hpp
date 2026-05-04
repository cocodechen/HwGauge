#pragma once

#ifdef HWGAUGE_USE_NPU

#include <string>

#ifdef HWGAUGE_USE_LOCAL_HTTP
#include <nlohmann/json.hpp>
#endif

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

#ifdef HWGAUGE_USE_LOCAL_HTTP
    inline void to_json(nlohmann::json& j, const NPULabel& l) {
        j = nlohmann::json{
            {"card_id", l.card_id},
            {"device_id", l.device_id},
            {"chip_type", l.chip_type},
            {"chip_name", l.chip_name}
        };
    }

    inline void to_json(nlohmann::json& j, const NPUMetrics& m) {
        j = nlohmann::json{
            {"freq_aicore", m.freq_aicore},
            {"freq_aicpu", m.freq_aicpu},
            {"freq_ctrlcpu", m.freq_ctrlcpu},
            {"util_aicore", m.util_aicore},
            {"util_aicpu", m.util_aicpu},
            {"util_ctrlcpu", m.util_ctrlcpu},
            {"util_vec", m.util_vec},
            {"mem_total_mb", m.mem_total_mb},
            {"mem_usage_mb", m.mem_usage_mb},
            {"util_mem", m.util_mem},
            {"util_membw", m.util_membw},
            {"freq_mem", m.freq_mem},
            {"chip_power", m.chip_power},
            {"health", m.health},
            {"temperature", m.temperature},
            {"voltage", m.voltage}
        };
    }
#endif
}

#endif