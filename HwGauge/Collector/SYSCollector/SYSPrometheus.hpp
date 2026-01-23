#pragma once

#if defined(HWGAUGE_USE_PROMETHEUS) && defined(__linux__)

#include "Collector/Prometheus.hpp"
#include "SYSMetrics.hpp"

namespace hwgauge
{
    class SYSPrometheus: public Prometheus<SYSLabel, SYSMetrics>
    {
    public:
        explicit SYSPrometheus(std::shared_ptr<prometheus::Registry> registry_);
        
        virtual ~SYSPrometheus() = default;

        void write(const std::vector<SYSLabel>& label_list, const std::vector<SYSMetrics>& metric_list);
        
    private:
        // 内存指标
        prometheus::Family<prometheus::Gauge>* memTotalFamily;
        prometheus::Family<prometheus::Gauge>* memUsedFamily;
        prometheus::Family<prometheus::Gauge>* memUtilizationFamily;
        
        // 磁盘指标
        prometheus::Family<prometheus::Gauge>* diskReadFamily;
        prometheus::Family<prometheus::Gauge>* diskWriteFamily;
        prometheus::Family<prometheus::Gauge>* diskUtilizationFamily;
        
        // 网络指标
        prometheus::Family<prometheus::Gauge>* netDownloadFamily;
        prometheus::Family<prometheus::Gauge>* netUploadFamily;
        
        // 功耗指标
        prometheus::Family<prometheus::Gauge>* systemPowerFamily;
    };
}

#endif