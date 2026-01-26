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
        // 1. 频率指标
        prometheus::Family<prometheus::Gauge>* aicore_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_freq_gauge_;
        prometheus::Family<prometheus::Gauge>* ctrlcpu_freq_gauge_;  // 新增
        
        // 2. 算力负载指标
        prometheus::Family<prometheus::Gauge>* aicore_util_gauge_;
        prometheus::Family<prometheus::Gauge>* aicpu_util_gauge_;
        prometheus::Family<prometheus::Gauge>* ctrlcpu_util_gauge_;  // 新增
        prometheus::Family<prometheus::Gauge>* vec_util_gauge_;
        
        // 3. 存储资源指标
        prometheus::Family<prometheus::Gauge>* mem_total_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_usage_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_util_gauge_;
        prometheus::Family<prometheus::Gauge>* membw_util_gauge_;
        prometheus::Family<prometheus::Gauge>* mem_freq_gauge_;  // 新增
        
        // 4. 功耗指标
        prometheus::Family<prometheus::Gauge>* chip_power_gauge_;
        
        // 5. 环境指标
        prometheus::Family<prometheus::Gauge>* health_gauge_;
        prometheus::Family<prometheus::Gauge>* temperature_gauge_;
        prometheus::Family<prometheus::Gauge>* voltage_gauge_;
    };
}

#endif