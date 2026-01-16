#ifdef HWGAUGE_USE_NVML

#include "GPUPrometheus.hpp"

namespace hwgauge
{
    GPUPrometheus::GPUPrometheus(std::shared_ptr<prometheus::Registry> registry)
    {
        // 创建指标族（Families）
        auto& registry_ref = *registry;
        gpuUtilizationFamily = &prometheus::BuildGauge()
            .Name("gpu_utilization_percent")
            .Help("GPU utilization percentage")
            .Register(registry_ref);
            
        memoryUtilizationFamily = &prometheus::BuildGauge()
            .Name("gpu_memory_utilization_percent")
            .Help("GPU memory utilization percentage")
            .Register(registry_ref);
            
        gpuFrequencyFamily = &prometheus::BuildGauge()
            .Name("gpu_frequency_mhz")
            .Help("GPU frequency in MHz")
            .Register(registry_ref);

        // 内存带宽利用率
        memoryFrequencyFamily = &prometheus::BuildGauge()
            .Name("gpu_memory_frequency_mhz")
            .Help("GPU memory frequency in MHz")
            .Register(registry_ref);

        // vector core利用率
        powerUsageFamily = &prometheus::BuildGauge()
            .Name("gpu_power_usage_watts")
            .Help("GPU power usage in watts")
            .Register(registry_ref);
    }

    void GPUPrometheus::write(const std::vector<GPULabel>& label_list,const std::vector<GPUMetrics>& metric_list)
    {
        // 更新每个设备的指标
        for (size_t i = 0; i < label_list.size(); i++)
        {
            const auto& label = label_list[i];
            const auto& metric = metric_list[i];
            
            // 构建标签
            std::map<std::string, std::string> labels = 
            {
                {"index", std::to_string(label.index)},
                {"name", label.name}
            };
            
            // 更新各个指标 - 使用Add()获取或创建带有标签的指标
            gpuUtilizationFamily->Add(labels).Set(metric.gpuUtilization);
            memoryUtilizationFamily->Add(labels).Set(metric.memoryUtilization);
            gpuFrequencyFamily->Add(labels).Set(metric.gpuFrequency);
            memoryFrequencyFamily->Add(labels).Set(metric.memoryFrequency);
            powerUsageFamily->Add(labels).Set(metric.powerUsage);
        }
    }
}

#endif