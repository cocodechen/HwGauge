#pragma once

#ifdef HWGAUGE_USE_NVML

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include "GPUPrometheus.hpp"
#include "GPUDatabase.hpp"

#include <utility>

namespace hwgauge
{
    template<typename T>
    class GPUCollector : public Collector
    {
    public:

#ifdef HWGAUGE_USE_POSTGRESQL
        explicit GPUCollector(std::shared_ptr<Registry> registry, T impl, bool dbEnable_, const ConnectionConfig& dbConfig, const std::string& dbTableName) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels()),
            dbEnable(dbEnable_)
        {
            if (dbEnable_)
            {
                db = std::make_unique<GPUDatabase>(dbConfig, dbTableName);
                // 完成npu静态数据的写入
                if(db)db->writeInfo(label_list);
            }
        }
#endif
        explicit GPUCollector(std::shared_ptr<Registry> registry, T impl) :
            Collector(registry), 
            impl(std::move(impl)), 
            pm(registry),
            label_list(labels()){}
        
        virtual ~GPUCollector() = default;

        void collect() override
        {
            // 获取标签（设备列表）和指标数据
            auto metric_list = sample(label_list);

			// 输出调试
			for(int i=0;i<label_list.size();i++)
			{
				outGPU(label_list[i],metric_list[i]);
			}

            // prometheus
            pm.write(label_list,metric_list);

#ifdef HWGAUGE_USE_POSTGRESQL
            // database
            if(dbEnable && db)db->writeMetric(label_list,metric_list);
#endif
        }

        std::string name() override { return impl.name(); }
        std::vector<GPULabel> labels() { return impl.labels(); }
        std::vector<GPUMetrics> sample(std::vector<GPULabel>&labels) { return impl.sample(labels); }

    private:
        T impl;
        GPUPrometheus pm;
        std::vector<GPULabel>label_list; //标签，此处只在构造时初始化一次，当然也可以每次采集周期都更新

#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<GPUDatabase> db;
#endif
    };
}

#endif
