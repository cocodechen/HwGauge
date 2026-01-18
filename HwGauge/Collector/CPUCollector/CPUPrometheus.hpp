#pragma once

#if defined(HWGAUGE_USE_INTEL_PCM) && defined(HWGAUGE_USE_PROMETHEUS)

#include "Collector/Prometheus.hpp"
#include "CPUMetrics.hpp"

namespace hwgauge
{
    class CPUPrometheus:public Prometheus<CPULabel,CPUMetrics>
    {
    public:
        explicit CPUPrometheus(std::shared_ptr<prometheus::Registry> registry_);
        
        virtual ~CPUPrometheus() = default;

        void write(const std::vector<CPULabel>& label_list,const std::vector<CPUMetrics>& metric_list);
    private:
        // Prometheus指标 - 使用Family<prometheus::Gauge>类型
        prometheus::Family<prometheus::Gauge>* cpuUtilizationFamily;
		prometheus::Family<prometheus::Gauge>* cpuFrequencyFamily;
		prometheus::Family<prometheus::Gauge>* c0ResidencyFamily;
		prometheus::Family<prometheus::Gauge>* c6ResidencyFamily;
		prometheus::Family<prometheus::Gauge>* powerUsageFamily;
		prometheus::Family<prometheus::Gauge>* memoryReadBandwidthFamily;
		prometheus::Family<prometheus::Gauge>* memoryWriteBandwidthFamily;
		prometheus::Family<prometheus::Gauge>* memoryPowerUsageFamily;
    };
}

#endif
