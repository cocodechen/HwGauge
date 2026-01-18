#if defined(HWGAUGE_USE_NPU) && defined(HWGAUGE_USE_PROMETHEUS)

#include "NPUPrometheus.hpp"

namespace hwgauge
{
    NPUPrometheus::NPUPrometheus(std::shared_ptr<prometheus::Registry> registry_)
        : Prometheus<NPULabel, NPUMetrics>(registry_)
    {
        // 创建指标族（Families）
        auto& registry_ref = *registry;

        // AICore利用率
        aicore_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicore_utilization_percent")
            .Help("NPU AI Core utilization percentage")
            .Register(registry_ref);
            
        // AICPU利用率
        aicpu_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicpu_utilization_percent")
            .Help("NPU AI CPU utilization percentage")
            .Register(registry_ref);
            
        // Mem利用率
        mem_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_utilization_percent")
            .Help("NPU memory utilization percentage")
            .Register(registry_ref);

        // 内存带宽利用率
        membw_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_memory_bandwidth_utilization_percent")
            .Help("NPU memory bandwidth utilization percentage")
            .Register(registry_ref);

        // vector core利用率
        vec_util_gauge_ = &prometheus::BuildGauge()
            .Name("npu_vector_core_utilization_percent")
            .Help("NPU vector core utilization percentage")
            .Register(registry_ref);

        // AICore频率
        aicore_freq_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicore_frequency_mhz")
            .Help("NPU AI Core frequency in MHz")
            .Register(registry_ref);
            
        // AICPU频率
        aicpu_freq_gauge_ = &prometheus::BuildGauge()
            .Name("npu_aicpu_frequency_mhz")
            .Help("NPU AI CPU frequency in MHz")
            .Register(registry_ref);
            
        // Mem频率
        mem_freq_gauge_ = &prometheus::BuildGauge()
            .Name("npu_mem_frequency_mhz")
            .Help("NPU mem frequency in MHz")
            .Register(registry_ref);

        // 功耗
        power_gauge_ = &prometheus::BuildGauge()
            .Name("npu_power_watts")
            .Help("NPU power consumption in watts")
            .Register(registry_ref);

        // 健康状态
        health_gauge_ = &prometheus::BuildGauge()
            .Name("npu_health") 
            .Help("NPU device health status (0:OK,1:WARN,2:ERROR,3:CRITICAL,0xFFFFFFFF:NOT_EXIST)")
            .Register(registry_ref);
            
        // 温度
        temperature_gauge_ = &prometheus::BuildGauge()
            .Name("npu_temperature_celsius")
            .Help("NPU temperature in Celsius")
            .Register(registry_ref);
            
        // 电压
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
            
            // 更新各个指标 - 使用Add()获取或创建带有标签的指标
            aicore_util_gauge_->Add(labels).Set(metric.util_aicore);
            aicpu_util_gauge_->Add(labels).Set(metric.util_aicpu);
            mem_util_gauge_->Add(labels).Set(metric.util_mem);
            membw_util_gauge_->Add(labels).Set(metric.util_membw);
            vec_util_gauge_->Add(labels).Set(metric.util_vec);

            aicore_freq_gauge_->Add(labels).Set(metric.freq_aicore);
            aicpu_freq_gauge_->Add(labels).Set(metric.freq_aicpu);
            mem_freq_gauge_->Add(labels).Set(metric.freq_mem);

            power_gauge_->Add(labels).Set(metric.power);

            health_gauge_->Add(labels).Set(metric.health);
            temperature_gauge_->Add(labels).Set(metric.temperature);
            voltage_gauge_->Add(labels).Set(metric.voltage);
        }
    }
}

#endif