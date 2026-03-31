#pragma once

#include <optional>
#include <mutex>

namespace hwgauge
{
    /*共享的上下文结构*/
    struct SharedPowerContext {
        std::optional<double> cpu_power;
        std::optional<double> memory_power;
        std::optional<double> gpu_power;
        std::optional<double> npu_power;

        // 计算总和的辅助函数
        double getTotalPower() const {
            return cpu_power.value_or(0.0) + 
                memory_power.value_or(0.0) +
                gpu_power.value_or(0.0) + 
                npu_power.value_or(0.0);
        }
    }sharedPower;
} 