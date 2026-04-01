#pragma once

#include <optional>

namespace hwgauge
{
    /*共享的上下文结构*/
    struct SharedPowerContext {
        std::optional<double> cpu_power;
        std::optional<double> memory_power;
        std::optional<double> gpu_power;
        std::optional<double> npu_power;

        // 重置函数
        inline void reSet() {
            cpu_power.reset();
            memory_power.reset();
            gpu_power.reset();
            npu_power.reset();
        }

        // 计算总和的辅助函数
        inline double getTotalPower() const {
            return cpu_power.value_or(0.0) + 
                memory_power.value_or(0.0) +
                gpu_power.value_or(0.0) + 
                npu_power.value_or(0.0);
        }
    };

    inline SharedPowerContext sharedPower; // 定义全局共享上下文，注意要加inline以避免多重定义问题
} 