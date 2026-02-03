#pragma once

#ifdef HWGAUGE_USE_CLUSTER

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
        ClusterImpl(const std::string& redisUri = "tcp://127.0.0.1:6379");
        ~ClusterImpl();

        // 禁止拷贝
        ClusterImpl(const ClusterImpl&) = delete;
        ClusterImpl& operator=(const ClusterImpl&) = delete;

        std::string name() { return "Cluster Info Collector"; }
        std::vector<ClusterLabel> labels();
        
        // 采样逻辑 (主线程调用)
        std::vector<ClusterMetrics> sample(std::vector<ClusterLabel>& labels);

        // 启动心跳线程
        // nodeId: 本机唯一标识
        // ttlSeconds: Key 的过期时间 (心跳间隔会自动设为 ttl / 2)
        void startHeartbeat(const std::string& nodeId, int ttlSeconds = 5);

        // 停止心跳线程
        void stopHeartbeat();

    private:
        // Redis 连接与断开
        bool ensureConnection();
        void disconnect();
        void parseUri(const std::string& uri, std::string& ip, int& port);

        // 实际发送心跳的内部函数
        void sendHeartbeatPayload(const std::string& nodeId, int ttlSeconds);

        // 统计活跃节点
        long long countActiveNodes();

        std::string redisUri_;
        redisContext* ctx_;
        bool isConnected_;
        const std::string heartbeatPattern_ = "cluster:node:*:heartbeat";

        // 线程安全与后台线程相关
        std::recursive_mutex redisMutex_; // 使用递归锁，防止同一线程内函数嵌套调用导致死锁
        std::thread heartbeatThread_;
        std::atomic<bool> keepRunning_;   // 控制线程退出
    };
}

#endif