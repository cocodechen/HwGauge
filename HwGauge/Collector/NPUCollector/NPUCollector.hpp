#pragma once

#ifdef HWGAUGE_USE_NPU

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include "NPUPrometheus.hpp"
#include "NPUDatabase.hpp"

#include <utility>

namespace hwgauge
{
    template<typename T>
    class NPUCollector : public Collector
    {
    public:
#ifdef HWGAUGE_USE_POSTGRESQL
        explicit NPUCollector(std::shared_ptr<Registry> registry, T impl, bool dbEnable_, const ConnectionConfig& dbConfig, const std::string& dbTableName) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels()),
            dbEnable(dbEnable_)
        {
            if (dbEnable_)
            {
                db = std::make_unique<NPUDatabase>(dbConfig, dbTableName);
                // 完成npu静态数据的写入
                if(db)db->writeInfo(label_list);
            }
        }
#endif
        explicit NPUCollector(std::shared_ptr<Registry> registry, T impl) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels())
            {}
        
        virtual ~NPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto metric_list = sample(label_list);
            // prometheus
            pm.write(label_list,metric_list);

#ifdef HWGAUGE_USE_POSTGRESQL
            // database
            if(dbEnable && db)db->writeMetric(label_list,metric_list);
#endif
        }

        std::string name() override { return impl.name(); }
        std::vector<NPULabel> labels() { return impl.labels(); }
        std::vector<NPUMetrics> sample(std::vector<NPULabel>&labels) { return impl.sample(labels); }

    private:
        T impl;
        NPUPrometheus pm;
        std::vector<NPULabel>label_list;

#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<NPUDatabase> db;
#endif
    };
}

#endif
