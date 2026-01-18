#pragma once

#if defined(HWGAUGE_USE_NPU) && defined(HWGAUGE_USE_PROMETHEUS)

#include "Collector/Prometheus.hpp"
#include "NPUMetrics.hpp"

namespace hwgauge
{
    class NPUPrometheus:public Prometheus<NPULabel,NPUMetrics>
    {
    public:
        explicit NPUPrometheus(std::shared_ptr<prometheus::Registry> registry_);
        
        virtual ~NPUPrometheus() = default;

        void write(const std::vector<NPULabel>& label_list,const std::vector<NPUMetrics>& metric_list);
    private:
        // Prometheus指标 - 使用Family<prometheus::Gauge>类型
        prometheus::Family<prometheus::Gauge>* aicore_util_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_util_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_util_gauge_;
        prometheus::Family<prometheus::Gauge>* membw_util_gauge_;
        prometheus::Family<prometheus::Gauge>* vec_util_gauge_;

        prometheus::Family<prometheus::Gauge>* aicore_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_freq_gauge_;

        prometheus::Family<prometheus::Gauge>* power_gauge_;

        prometheus::Family<prometheus::Gauge>* health_gauge_;
        prometheus::Family<prometheus::Gauge>* temperature_gauge_;
        prometheus::Family<prometheus::Gauge>* voltage_gauge_;
    };
}

#endif