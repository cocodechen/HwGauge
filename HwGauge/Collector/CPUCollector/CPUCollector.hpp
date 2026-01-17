#pragma once

#ifdef HWGAUGE_USE_INTEL_PCM

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include "CPUPrometheus.hpp"
#include "CPUDatabase.hpp"

#include <utility>

namespace hwgauge
{
    template<typename T>
    class CPUCollector : public Collector
    {
    public:

#ifdef HWGAUGE_USE_POSTGRESQL
        explicit CPUCollector(std::shared_ptr<Registry> registry, T impl, bool dbEnable_, const ConnectionConfig& dbConfig, const std::string& dbTableName) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels()),
            dbEnable(dbEnable_)
        {
            if (dbEnable_)
            {
                db = std::make_unique<CPUDatabase>(dbConfig, dbTableName);
                // 完成npu静态数据的写入
                if(db)db->writeInfo(label_list);
            }
        }
#endif
        explicit CPUCollector(std::shared_ptr<Registry> registry, T impl) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels()){}
        
        virtual ~CPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto metric_list = sample(label_list);

			// 输出调试
			for(int i=0;i<label_list.size();i++)
			{
				outCPU(label_list[i],metric_list[i]);
			}

            // prometheus
            pm.write(label_list,metric_list);

#ifdef HWGAUGE_USE_POSTGRESQL
            // database
            if(dbEnable && db)db->writeMetric(label_list,metric_list);
#endif
        }

        std::string name() override { return impl.name(); }
        std::vector<CPULabel> labels() { return impl.labels(); }
        std::vector<CPUMetrics> sample(std::vector<CPULabel>&labels) { return impl.sample(labels); }

    private:
        T impl;
        CPUPrometheus pm;
        std::vector<CPULabel>label_list; //标签，此处只在构造时初始化一次，当然也可以每次采集周期都更新

#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<CPUDatabase> db;
#endif
    };
}

#endif
