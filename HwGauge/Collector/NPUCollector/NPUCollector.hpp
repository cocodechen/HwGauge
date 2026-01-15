#pragma once

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include "NPUPrometheus.hpp"
#include "NPUDatabase.hpp"

#include "spdlog/spdlog.h"
#include <utility>

namespace hwgauge
{
    template<typename T>
    class NPUCollector : public Collector
    {
    public:
        explicit NPUCollector(std::shared_ptr<Registry> registry, T impl, bool dbEnable_, const ConnectionConfig& dbConfig, const std::string& dbTableName) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            dbEnable(dbEnable_)
        {
            if (dbEnable_)
            {
                db = std::make_unique<NPUDatabase>(dbConfig, dbTableName);
                // 完成npu静态数据的写入
                if(db)
                {
                    auto label_list = labels();
                    auto info_list = getInfo();
                    db->writeInfo(label_list,info_list);
                }
            }
        }
        
        virtual ~NPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto label_list = labels();
            auto metric_list = sample();
            if (label_list.size() != metric_list.size())
            {
                spdlog::warn("[NPUCollector] Label list and metric list size mismatch");
                return;
            }
            // prometheus
            pm.write(label_list,metric_list);
            // database
            if(dbEnable && db)db->writeMetric(label_list,metric_list);

        }

        std::string name() override { return impl.name(); }
        std::vector<NPULabel> labels() { return impl.labels(); }
        std::vector<NPUMetrics> sample() { return impl.sample(); }
        std::vector<NPUInfo> getInfo() { return impl.getInfo(); }

    private:
        T impl;

        NPUPrometheus pm;
        
        bool dbEnable;
        std::unique_ptr<NPUDatabase> db;
    };
}
