#pragma once

#ifdef HWGAUGE_USE_CLUSTER

#include "Collector/Common/Config.hpp"
#include "ClusterMetrics.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <hiredis/hiredis.h>

namespace hwgauge
{
    class ClusterImpl
    {
    public:
        // 修改构造函数参数
        ClusterImpl(const ClusterConfig& config);
        ~ClusterImpl();

        // 禁止拷贝
        ClusterImpl(const ClusterImpl&) = delete;
        ClusterImpl& operator=(const ClusterImpl&) = delete;

        std::string name() { return "Cluster Info Collector"; }
        std::vector<ClusterLabel> labels();
        
        // 采样逻辑 (主线程调用)
        std::vector<ClusterMetrics> sample(std::vector<ClusterLabel>& labels);

        // 启动心跳线程 (参数已移除，使用内部 config)
        void startHeartbeat();

        // 停止心跳线程
        void stopHeartbeat();

    private:
        // Redis 连接与断开
        bool ensureConnection();
        void disconnect();
        // parseUri 已移除，不再需要

        // 实际发送心跳的内部函数 (参数已移除，使用内部 config)
        void sendHeartbeatPayload();

        // 统计活跃节点
        long long countActiveNodes();

        ClusterConfig config_; // 替换原有的 redisUri_
        redisContext* ctx_;
        bool isConnected_;
        const std::string heartbeatPattern_ = "cluster:node:*:heartbeat";

        // 线程安全与后台线程相关
        std::recursive_mutex redisMutex_; 
        std::thread heartbeatThread_;
        std::atomic<bool> keepRunning_;   
    };
}

#endif