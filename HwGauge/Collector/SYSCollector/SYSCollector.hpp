#pragma once

#ifdef __linux__

#include "Collector/Collector.hpp"
#include "SYSMetrics.hpp"
#include "SYSImpl.hpp"
#include "SYSPrometheus.hpp"
#include "SYSDatabase.hpp"

namespace hwgauge
{
    class SYSCollector : public Collector
    {
    public:
        explicit SYSCollector(const CollectorConfig& cfg)
            : impl(std::make_unique<SYSImpl>()),
            label_list(labels()),
            outTer(cfg.outTer)
        {
#ifdef HWGAUGE_USE_PROMETHEUS
            pmEnable = cfg.pmEnable;
            if (pmEnable)pm = std::make_unique<SYSPrometheus>(cfg.registry);
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
            dbEnable = cfg.dbEnable;
            if (dbEnable)
            {
                db = std::make_unique<SYSDatabase>(cfg.dbConfig, cfg.dbTableName);
                db->writeInfo(label_list);
            }
#endif
        }
        
        virtual ~SYSCollector() = default;

        void collect(const std::string&cur_time) override
        {
            // 获取标签（设备列表）和指标数据
            auto metric_list = sample(label_list);

			// 输出调试
            if(outTer)
            {
                std::cout<<"["<<cur_time<<"]"<<std::endl;
                for(int i=0;i<label_list.size();i++)
                    outSYS(label_list[i],metric_list[i]);
            }
#ifdef HWGAUGE_USE_PROMETHEUS
            // prometheus
            if(pmEnable && pm)pm->write(label_list,metric_list);
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
            // database
            if(dbEnable && db)db->writeMetric(cur_time,label_list,metric_list);
#endif
        }

        std::string name() override { return impl->name(); }
        std::vector<SYSLabel> labels() { return impl->labels(); }
        std::vector<SYSMetrics> sample(std::vector<SYSLabel>&labels) { return impl->sample(labels); }

    private:
        std::unique_ptr<SYSImpl> impl;      //由于类中存在原子变量，因此不能移动，故在此构造
        std::vector<SYSLabel>label_list;    //标签，此处只在构造时初始化一次，当然也可以每次采集周期都更新
        bool outTer;
#ifdef HWGAUGE_USE_PROMETHEUS
        bool pmEnable;
        std::unique_ptr<SYSPrometheus> pm;
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<SYSDatabase> db;
#endif
    };
}

#endif
