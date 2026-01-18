#pragma once

#ifdef HWGAUGE_USE_NPU

#include "Collector/Collector.hpp"
#include "NPUMetrics.hpp"
#include "NPUPrometheus.hpp"
#include "NPUDatabase.hpp"

namespace hwgauge
{
    template<typename T>
    class NPUCollector : public Collector
    {
    public:
        explicit NPUCollector(T impl, const CollectorConfig& cfg)
            : impl(std::move(impl)),
            label_list(labels()),
            outTer(cfg.outTer)
        {
#ifdef HWGAUGE_USE_PROMETHEUS
            pmEnable = cfg.pmEnable;
            if (pmEnable)pm = std::make_unique<NPUPrometheus>(cfg.registry);
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
            dbEnable = cfg.dbEnable;
            if (dbEnable)
            {
                db = std::make_unique<NPUDatabase>(cfg.dbConfig, cfg.dbTableName);
                db->writeInfo(label_list);
            }
#endif
        }
        
        virtual ~NPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto metric_list = sample(label_list);

			// 输出调试
            if(outTer)
            {
                for(int i=0;i<label_list.size();i++)
                    outNPU(label_list[i],metric_list[i]);
            }
#ifdef HWGAUGE_USE_PROMETHEUS
            // prometheus
            if(pmEnable && pm)pm->write(label_list,metric_list);
#endif
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
        std::vector<NPULabel>label_list; //标签，此处只在构造时初始化一次，当然也可以每次采集周期都更
        bool outTer;
#ifdef HWGAUGE_USE_PROMETHEUS
        bool pmEnable;
        std::unique_ptr<NPUPrometheus> pm;
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<NPUDatabase> db;
#endif
    };
}

#endif
