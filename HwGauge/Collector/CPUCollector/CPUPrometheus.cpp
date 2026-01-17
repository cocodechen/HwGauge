#ifdef HWGAUGE_USE_INTEL_PCM

#include "CPUPrometheus.hpp"

namespace hwgauge
{
    CPUPrometheus::CPUPrometheus(std::shared_ptr<prometheus::Registry> registry)
    {
        // 创建指标族（Families）
        auto& registry_ref = *registry;
        cpuUtilizationFamily = &prometheus::BuildGauge()
            .Name("cpu_utilization_percent")
            .Help("CPU utilization percentage")
            .Register(registry_ref);
            
        cpuFrequencyFamily = &prometheus::BuildGauge()
            .Name("cpu_frequency_mhz")
            .Help("CPU frequency in MHz")
            .Register(registry_ref);
            
        c0ResidencyFamily = &prometheus::BuildGauge()
            .Name("cpu_c0_residency_percent")
            .Help("CPU C0 state residency percentage")
            .Register(registry_ref);

        c6ResidencyFamily = &prometheus::BuildGauge()
            .Name("cpu_c6_residency_percent")
            .Help("CPU C6 state residency percentage")
            .Register(registry_ref);

        powerUsageFamily = &prometheus::BuildGauge()
            .Name("cpu_power_usage_watts")
            .Help("CPU power usage in watts")
            .Register(registry_ref);
        
        memoryReadBandwidthFamily = &prometheus::BuildGauge()
            .Name("memory_read_bandwidth_mbps")
            .Help("Memory read bandwidth in MB/s")
            .Register(registry_ref);
        
        memoryWriteBandwidthFamily = &prometheus::BuildGauge()
            .Name("memory_write_bandwidth_mbps")
            .Help("Memory write bandwidth in MB/s")
            .Register(registry_ref);

        memoryPowerUsageFamily = &prometheus::BuildGauge()
            .Name("memory_power_usage_watts")
            .Help("Memory power usage in watts")
            .Register(registry_ref);
    }

    void CPUPrometheus::write(const std::vector<CPULabel>& label_list,const std::vector<CPUMetrics>& metric_list)
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
            cpuUtilizationFamily->Add(labels).Set(metric.cpuUtilization);
            cpuFrequencyFamily->Add(labels).Set(metric.cpuFrequency);
            c0ResidencyFamily->Add(labels).Set(metric.c0Residency);
            c6ResidencyFamily->Add(labels).Set(metric.c6Residency);
            powerUsageFamily->Add(labels).Set(metric.powerUsage);
            memoryReadBandwidthFamily->Add(labels).Set(metric.memoryReadBandwidth);
            memoryWriteBandwidthFamily->Add(labels).Set(metric.memoryWriteBandwidth);
            memoryPowerUsageFamily->Add(labels).Set(metric.memoryPowerUsage);
        }
    }
}

#endif