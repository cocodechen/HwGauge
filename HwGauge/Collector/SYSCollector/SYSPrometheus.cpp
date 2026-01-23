#if defined(HWGAUGE_USE_PROMETHEUS) && defined(__linux__)

#include "SYSPrometheus.hpp"

namespace hwgauge
{
    SYSPrometheus::SYSPrometheus(std::shared_ptr<prometheus::Registry> registry_)
        : Prometheus<SYSLabel, SYSMetrics>(registry_)
    {
        // 创建指标族（Families）
        auto& registry_ref = *registry;
        
        // 内存指标
        memTotalFamily = &prometheus::BuildGauge()
            .Name("system_memory_total_gb")
            .Help("Total system memory in GB")
            .Register(registry_ref);
            
        memUsedFamily = &prometheus::BuildGauge()
            .Name("system_memory_used_gb")
            .Help("Used system memory in GB")
            .Register(registry_ref);
            
        memUtilizationFamily = &prometheus::BuildGauge()
            .Name("system_memory_utilization_percent")
            .Help("System memory utilization percentage")
            .Register(registry_ref);
        
        // 磁盘指标
        diskReadFamily = &prometheus::BuildGauge()
            .Name("system_disk_read_mbps")
            .Help("System disk read throughput in MB/s")
            .Register(registry_ref);
            
        diskWriteFamily = &prometheus::BuildGauge()
            .Name("system_disk_write_mbps")
            .Help("System disk write throughput in MB/s")
            .Register(registry_ref);
            
        diskUtilizationFamily = &prometheus::BuildGauge()
            .Name("system_disk_utilization_percent")
            .Help("Maximum disk utilization percentage across all physical disks")
            .Register(registry_ref);
        
        // 网络指标
        netDownloadFamily = &prometheus::BuildGauge()
            .Name("system_network_download_mbps")
            .Help("System network download bandwidth in MB/s")
            .Register(registry_ref);
            
        netUploadFamily = &prometheus::BuildGauge()
            .Name("system_network_upload_mbps")
            .Help("System network upload bandwidth in MB/s")
            .Register(registry_ref);
        
        // 功耗指标
        systemPowerFamily = &prometheus::BuildGauge()
            .Name("system_power_watts")
            .Help("Total system power consumption in watts")
            .Register(registry_ref);
    }

    void SYSPrometheus::write(const std::vector<SYSLabel>& label_list, const std::vector<SYSMetrics>& metric_list)
    {
        // 更新每个标签对应的指标
        for (size_t i = 0; i < label_list.size(); i++)
        {
            const auto& label = label_list[i];
            const auto& metric = metric_list[i];
            
            // 构建标签
            std::map<std::string, std::string> labels = 
            {
                {"name", label.name}
                // 系统监控通常只有一个全局标签，但保持扩展性
            };
            
            // 更新内存指标
            memTotalFamily->Add(labels).Set(metric.memTotalGB);
            memUsedFamily->Add(labels).Set(metric.memUsedGB);
            memUtilizationFamily->Add(labels).Set(metric.memUtilizationPercent);
            
            // 更新磁盘指标
            diskReadFamily->Add(labels).Set(metric.diskReadMBps);
            diskWriteFamily->Add(labels).Set(metric.diskWriteMBps);
            diskUtilizationFamily->Add(labels).Set(metric.maxDiskUtilPercent);
            
            // 更新网络指标
            netDownloadFamily->Add(labels).Set(metric.netDownloadMBps);
            netUploadFamily->Add(labels).Set(metric.netUploadMBps);
            
            // 更新功耗指标
            systemPowerFamily->Add(labels).Set(metric.systemPowerWatts);
        }
    }
}

#endif