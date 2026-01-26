#if defined(HWGAUGE_USE_NPU) && defined(HWGAUGE_USE_PROMETHEUS)

#include "NPUPrometheus.hpp"

namespace hwgauge
{
    NPUPrometheus::NPUPrometheus(std::shared_ptr<prometheus::Registry> registry_)
        : Prometheus<NPULabel, NPUMetrics>(registry_)
    {
        // 创建指标族（Families）
        auto& registry_ref = *registry;

        // 1. 频率指标
        aicore_freq_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicore_frequency_mhz")
            .Help("NPU AI Core frequency in MHz")
            .Register(registry_ref);
            
        aicpu_freq_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicpu_frequency_mhz")
            .Help("NPU AI CPU frequency in MHz")
            .Register(registry_ref);
            
        // 2. 算力负载指标
        aicore_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicore_utilization_percent")
            .Help("NPU AI Core utilization percentage")
            .Register(registry_ref);
            
        aicpu_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicpu_utilization_percent")
            .Help("NPU AI CPU utilization percentage")
            .Register(registry_ref);
            
        vec_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_vector_core_utilization_percent")
            .Help("NPU vector core utilization percentage")
            .Register(registry_ref);

        // 3. 存储资源指标
        mem_total_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_total_mb")
            .Help("NPU total memory in MB")
            .Register(registry_ref);
            
        mem_usage_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_usage_mb")
            .Help("NPU memory usage in MB")
            .Register(registry_ref);
            
        mem_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_utilization_percent")
            .Help("NPU memory utilization percentage")
            .Register(registry_ref);

        membw_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_bandwidth_utilization_percent")
            .Help("NPU memory bandwidth utilization percentage")
            .Register(registry_ref);

        // 4. 功耗指标
        chip_power_gauge_ = &prometheus::BuildGauge()
            .Name("npu_chip_power_watts")
            .Help("NPU chip power consumption in watts")
            .Register(registry_ref);
            
        mcu_power_gauge_ = &prometheus::BuildGauge()
            .Name("npu_mcu_power_watts")
            .Help("NPU MCU power consumption in watts")
            .Register(registry_ref);

        // 5. 环境指标
        health_gauge_ = &prometheus::BuildGauge()
            .Name("npu_health") 
            .Help("NPU device health status (0:OK,1:WARN,2:ERROR,3:CRITICAL,0xFFFFFFFF:NOT_EXIST)")
            .Register(registry_ref);
            
        temperature_gauge_ = &prometheus::BuildGauge()
            .Name("npu_temperature_celsius")
            .Help("NPU temperature in Celsius")
            .Register(registry_ref);
            
        voltage_gauge_ = &prometheus::BuildGauge()
            .Name("npu_voltage_volts")
            .Help("NPU voltage in Volts")
            .Register(registry_ref);
    }

    void NPUPrometheus::write(const std::vector<NPULabel>& label_list,const std::vector<NPUMetrics>& metric_list)
    {
        // 更新每个设备的指标
        for (size_t i = 0; i < label_list.size(); i++)
        {
            const auto& label = label_list[i];
            const auto& metric = metric_list[i];
            
            // 构建标签
            std::map<std::string, std::string> labels = 
            {
                {"card_id", std::to_string(label.card_id)},
                {"device_id", std::to_string(label.device_id)}
            };
            
            // 1. 更新频率指标
            aicore_freq_gauge_->Add(labels).Set(metric.freq_aicore);
            aicpu_freq_gauge_->Add(labels).Set(metric.freq_aicpu);
            
            // 2. 更新算力负载指标
            aicore_util_gauge_->Add(labels).Set(metric.util_aicore);
            aicpu_util_gauge_->Add(labels).Set(metric.util_aicpu);
            vec_util_gauge_->Add(labels).Set(metric.util_vec);
            
            // 3. 更新存储资源指标
            mem_total_gauge_->Add(labels).Set(metric.mem_total_mb);
            mem_usage_gauge_->Add(labels).Set(metric.mem_usage_mb);
            mem_util_gauge_->Add(labels).Set(metric.util_mem);
            membw_util_gauge_->Add(labels).Set(metric.util_membw);
            
            // 4. 更新功耗指标
            chip_power_gauge_->Add(labels).Set(metric.chip_power);
            mcu_power_gauge_->Add(labels).Set(metric.mcu_power);
            
            // 5. 更新环境指标
            health_gauge_->Add(labels).Set(metric.health);
            temperature_gauge_->Add(labels).Set(metric.temperature);
            voltage_gauge_->Add(labels).Set(metric.voltage);
        }
    }
}

#endif