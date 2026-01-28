#pragma once

#include "Collector/Collector.hpp"
#include <memory>
#include <vector>
#include <iostream>

namespace hwgauge
{
    // 辅助打印函数，需要在外部重载
    template<typename L, typename M>
    void printMetric(const L& label, const M& metric);

    /**
     * @tparam LabelT : 标签结构 (GPULabel)
     * @tparam MetricT: 指标结构 (GPUMetrics)
     * @tparam ImplT  : 硬件实现 (NVML, CPUImpl)
     * @tparam DbT    : 数据库处理类 (GPUDatabase, CPUDatabase)
     * @tparam CsvT   : CSV日志类 (GPUCsvLogger, CPUCsvLogger)
     * @tparam PromT  : Prometheus类 (GPUPrometheus, ...)
     */
    template <
        typename LabelT, 
        typename MetricT,
        typename ImplT, 
        typename DbT,
        typename CsvT,
        typename PromT
    >
    class DeviceCollector : public Collector
    {
    public:
        explicit DeviceCollector(const CollectorConfig& cfg)
            : impl(std::make_unique<ImplT>()),
              outTer(cfg.outTer),
              outFile(cfg.outFile)
        {
            label_list = labels();

            if(outFile)
            {
                cl = std::make_unique<CsvT>(cfg.filepath);
                cl->init();
            }
#ifdef HWGAUGE_USE_PROMETHEUS
            pmEnable = cfg.pmEnable;
            if (pmEnable)pm = std::make_unique<PromT>(cfg.registry);
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
            dbEnable = cfg.dbEnable;
            if (dbEnable)
            {
                db = std::make_unique<DbT>(cfg.dbConfig, cfg.dbTableName);
                db->init(label_list);
            }
#endif
        }
        
        virtual ~DeviceCollector() = default;

        void collect(const std::string& cur_time) override
        {
            auto metric_list = sample(label_list);

            if(outTer) {
                for(size_t i=0; i<label_list.size(); i++) 
                    printMetric(label_list[i], metric_list[i]);
            }

            if(outFile && cl) cl->write(cur_time, label_list, metric_list);

#ifdef HWGAUGE_USE_PROMETHEUS
            if(pmEnable && pm) pm->write(label_list, metric_list);
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
            if(dbEnable && db) db->writeMetric(cur_time, label_list, metric_list);
#endif
        }

        std::string name() override { return impl->name(); }
        std::vector<LabelT> labels() { return impl->labels(); }
        std::vector<MetricT> sample(std::vector<LabelT>& labels) { return impl->sample(labels); }

    private:
        std::unique_ptr<ImplT> impl;
        std::vector<LabelT> label_list;

        bool outTer;

        bool outFile;
        std::unique_ptr<CsvT> cl; 
#ifdef HWGAUGE_USE_PROMETHEUS
        bool pmEnable;
        std::unique_ptr<PromT> pm;
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable;
        std::unique_ptr<DbT> db;
#endif
    };
}