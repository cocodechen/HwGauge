#pragma once

#if defined(HWGAUGE_USE_NVML) && defined(HWGAUGE_USE_PROMETHEUS)

#include "Collector/Prometheus.hpp"
#include "GPUMetrics.hpp"

namespace hwgauge
{
    class GPUPrometheus:public Prometheus<GPULabel,GPUMetrics>
    {
    public:
        explicit GPUPrometheus(std::shared_ptr<prometheus::Registry> registry_);
        
        virtual ~GPUPrometheus() = default;

        void write(const std::vector<GPULabel>& label_list,const std::vector<GPUMetrics>& metric_list);
    private:
        // Prometheus指标 - 使用Family<prometheus::Gauge>类型
        prometheus::Family<prometheus::Gauge>* gpuUtilizationFamily;
		prometheus::Family<prometheus::Gauge>* memoryUtilizationFamily;
		prometheus::Family<prometheus::Gauge>* gpuFrequencyFamily;
		prometheus::Family<prometheus::Gauge>* memoryFrequencyFamily;
		prometheus::Family<prometheus::Gauge>* powerUsageFamily;
    };
}

#endif
