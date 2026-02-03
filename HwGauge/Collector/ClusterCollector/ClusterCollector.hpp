#pragma once
#ifdef HWGAUGE_USE_CLUSTER

#include "Collector/Base/Collector.hpp"
#include "ClusterImpl.hpp"
#include <memory>
#include <vector>
#include <iostream>

namespace hwgauge
{
    // 辅助打印函数，需要在外部重载
    inline void printMetric(const ClusterLabel& l, const ClusterMetrics& m)
    {
        std::cout << "Cluster{ "
                << "Name: " << l.clusterName << ", "
                << "ActiveNodes: " << m.activeNodeCount << ", "
                << "RedisLatency: " << m.redisLatencyMs << " ms, "
                << "RedisConnected: " << m.redisConnected
                << " }\n";
    }

    class ClusterCollector : public Collector
    {
    public:
        explicit ClusterCollector(const CollectorConfig& cfg)
            : impl(std::make_unique<ClusterImpl>(cfg.redisUri)),
              outTer(cfg.outTer)
        {
            label_list = labels();
            if(cfg.hbEnable)impl->startHeartbeat(cfg.nodeId, cfg.ttlSeconds);
        }
        
        virtual ~ClusterCollector() = default;

        void collect(const std::string& cur_time) override
        {
            auto metric_list = sample(label_list);

            if(outTer) {
                for(size_t i=0; i<label_list.size(); i++) 
                    printMetric(label_list[i], metric_list[i]);
            }
        }

        std::string name() override { return impl->name(); }
        std::vector<ClusterLabel> labels() { return impl->labels(); }
        std::vector<ClusterMetrics> sample(std::vector<ClusterLabel>& labels) { return impl->sample(labels); }

    private:
        std::unique_ptr<ClusterImpl> impl;
        std::vector<ClusterLabel> label_list;

        bool outTer;
    };
}

#endif