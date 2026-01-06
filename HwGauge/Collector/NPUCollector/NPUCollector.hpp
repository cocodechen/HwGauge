#pragma once

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include <string>
#include <utility>
#include <vector>

namespace hwgauge
{
    struct NPULabel
    {
        int card_id;
        int device_id;
    };
    struct NPUMetric
    {
        //利用率（%）
        uint32_t util_aicore; //AICore
        uint32_t util_aicpu; //AICPU
        uint32_t util_mem; //Mem
        //频率（MHz）
        uint32_t aicore_freq; //AICore
        uint32_t aicpu_freq; //AICPU
        uint32_t mem_freq; //Mem
        //功耗（W）
        double power;

        //其他
        //健康状态 (0: OK, 1: WARN, 2: ERROR, 3: CRITICAL, 0xFFFFFFFF: NOT_EXIST)
        uint32_t health;
        //温度（C）
        int32_t temperature;
        //电压（V）
        double voltage;
    };

    template<typename T>
    class NPUCollector : public Collector
    {
    public:
        explicit NPUCollector(std::shared_ptr<Registry> registry, T impl) :
            Collector(registry), impl(std::move(impl))
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
        
        virtual ~NPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto label_list = labels();
            auto metric_list = sample();
            
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

                aicore_freq_gauge_->Add(labels).Set(metric.aicore_freq);
                aicpu_freq_gauge_->Add(labels).Set(metric.aicpu_freq);
                mem_freq_gauge_->Add(labels).Set(metric.mem_freq);

                power_gauge_->Add(labels).Set(metric.power);

                health_gauge_->Add(labels).Set(metric.health);
                temperature_gauge_->Add(labels).Set(metric.temperature);
                voltage_gauge_->Add(labels).Set(metric.voltage);
            }
        }

        std::string name() override { return impl.name(); }
        std::vector<NPULabel> labels() { return impl.labels(); }
        std::vector<NPUMetrics> sample() { return impl.sample(); }

    private:
        T impl;
        
        // Prometheus指标 - 使用Family<prometheus::Gauge>类型
        prometheus::Family<prometheus::Gauge>* aicore_util_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_util_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_util_gauge_;

        prometheus::Family<prometheus::Gauge>* aicore_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_freq_gauge_;

        prometheus::Family<prometheus::Gauge>* power_gauge_;

        prometheus::Family<prometheus::Gauge>* health_gauge_;
        prometheus::Family<prometheus::Gauge>* temperature_gauge_;
        prometheus::Family<prometheus::Gauge>* voltage_gauge_;
    };
}

#endif // NPU_COLLECTOR_H