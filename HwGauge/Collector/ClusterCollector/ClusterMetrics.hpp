#pragma once

#ifdef HWGAUGE_USE_CLUSTER

#include <string>

namespace hwgauge
{
    struct ClusterLabel
    {
        std::string clusterName;
    };

    struct ClusterMetrics
    {
        double activeNodeCount;      // 活跃节点数量 (基于 Redis 心跳统计)
        double redisLatencyMs;       // 连接 Redis 的延迟 (毫秒)
        double redisConnected;       // 1.0 表示连接正常, 0.0 表示断开

        // 预留扩展 (例如从 Redis 聚合读取的总算力)

        ClusterMetrics() 
            : activeNodeCount(0.0), 
              redisLatencyMs(-1.0), 
              redisConnected(0.0)
        {}
    };
}

#endif