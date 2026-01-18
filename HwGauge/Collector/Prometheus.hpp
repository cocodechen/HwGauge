#pragma once

#ifdef HWGAUGE_USE_PROMETHEUS

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "prometheus/exposer.h"
#include <memory>
#include <utility>
#include <vector>

namespace hwgauge
{
    template<typename LabelType, typename MetricsType>
    class Prometheus
    {
    public:
        explicit Prometheus(std::shared_ptr<prometheus::Registry> registry_)
            : registry(std::move(registry_)){}
        
        virtual ~Prometheus() = default;

        virtual void write(const std::vector<LabelType>& label_list,
                          const std::vector<MetricsType>& metric_list) = 0;

    protected:
        std::shared_ptr<prometheus::Registry> registry;
    };
}

#endif