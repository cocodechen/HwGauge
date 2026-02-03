#ifdef HWGAUGE_USE_CLUSTER

#include "ClusterImpl.hpp"
#include "Collector/Common/Exception.hpp" 

#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include <thread>

namespace hwgauge
{
    ClusterImpl::ClusterImpl(const std::string& redisUri)
        : redisUri_(redisUri), ctx_(nullptr), isConnected_(false), keepRunning_(false)
    {
        spdlog::info("Initializing ClusterImpl with URI: {}", redisUri_);
        // 构造函数中立即尝试连接
        // 这里的锁其实不是必须的（因为构造时没有竞争），但为了保持一致性加上
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (!ensureConnection())
        {
            // 如果起步就失败，根据要求抛出 FatalError
            spdlog::error("Fatal Error: Failed to establish initial connection to Redis at {}", redisUri_);
            throw FatalError("Initial Redis connection failed");
        }
        spdlog::info("Successfully connected to Redis at startup.");
    }

    ClusterImpl::~ClusterImpl()
    {
        spdlog::info("Shutting down ClusterImpl...");
        stopHeartbeat(); // 析构前必须停止线程
        disconnect();
    }

    std::vector<ClusterLabel> ClusterImpl::labels()
    {
        return {{"Global-Cluster"}};
    }

    void ClusterImpl::disconnect()
    {
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
            spdlog::debug("Redis context freed.");
        }
        isConnected_ = false;
    }

    void ClusterImpl::parseUri(const std::string& uri, std::string& ip, int& port)
    {
        std::string cleanUri = uri;
        const std::string prefix = "tcp://";
        if (cleanUri.find(prefix) == 0) {
            cleanUri = cleanUri.substr(prefix.length());
        }
        size_t colonPos = cleanUri.find(':');
        if (colonPos != std::string::npos) {
            ip = cleanUri.substr(0, colonPos);
            try {
                port = std::stoi(cleanUri.substr(colonPos + 1));
            } catch (...) {
                port = 6379;
                spdlog::warn("Invalid port in URI [{}], defaulting to 6379", uri);
            }
        } else {
            ip = cleanUri;
            port = 6379;
        }
    }

    // 内部连接函数：返回 bool，不抛出异常，供 sample 和 heartbeat 共享
    bool ClusterImpl::ensureConnection()
    {
        // 调用此函数前，外部应该已经持有锁，或者处于单线程环境(构造函数)
        // 但为了安全，再次利用 recursive_mutex 的特性
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (ctx_ && isConnected_) return true;

        // 清理旧连接
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
        }
        std::string ip;
        int port;
        parseUri(redisUri_, ip, port);

        struct timeval timeout = { 1, 0 }; // 连接超时 1秒
        ctx_ = redisConnectWithTimeout(ip.c_str(), port, timeout);
        if (ctx_ == nullptr || ctx_->err) {
            std::string errStr = (ctx_) ? ctx_->errstr : "can't allocate context";
            spdlog::warn("Redis connection failed to {}:{}. Error: {}", ip, port, errStr);
            
            if (ctx_) {
                redisFree(ctx_);
                ctx_ = nullptr;
            }
            isConnected_ = false;
            return false;
        }
        
        // 设置读写超时 500ms
        struct timeval io_timeout = { 0, 500000 }; 
        redisSetTimeout(ctx_, io_timeout);

        isConnected_ = true;
        spdlog::debug("Redis re-connected successfully.");
        return true;
    }

    void ClusterImpl::startHeartbeat(const std::string& nodeId, int ttlSeconds)
    {
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (keepRunning_)
        {
            spdlog::warn("Heartbeat thread already running.");
            return; 
        }
        spdlog::info("Starting heartbeat thread for NodeID: {}, TTL: {}s", nodeId, ttlSeconds);
        keepRunning_ = true;
        heartbeatThread_ = std::thread([this, nodeId, ttlSeconds]() {
            int intervalMs = (ttlSeconds * 1000) / 2;
            if (intervalMs < 100) intervalMs = 100;

            while (keepRunning_) {
                {
                    // 只在发送数据的瞬间加锁
                    std::lock_guard<std::recursive_mutex> lock(this->redisMutex_);
                    this->sendHeartbeatPayload(nodeId, ttlSeconds);
                }
                
                // 睡眠等待 (检查 keepRunning 以便快速退出)
                for (int i = 0; i < 10 && keepRunning_; ++i) {
                     std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs / 10));
                }
            }
            spdlog::info("Heartbeat thread stopped.");
        });
    }

    void ClusterImpl::stopHeartbeat()
    {
        keepRunning_ = false;
        if (heartbeatThread_.joinable()) {
            heartbeatThread_.join();
        }
    }

    void ClusterImpl::sendHeartbeatPayload(const std::string& nodeId, int ttlSeconds)
    {
        // 这里不能抛出异常，否则线程会挂掉，只能记录日志并静默失败等待重试
        if (!ensureConnection()) {
            // ensureConnection 内部已经 warn 过了
            return;
        }

        std::string key = "cluster:node:" + nodeId + ":heartbeat";
        // SET key 1 EX ttl
        redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d", key.c_str(), "1", ttlSeconds);

        if (reply == nullptr) {
            spdlog::warn("Heartbeat failed: IO Error. Connection marked down.");
            isConnected_ = false; 
            if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
            return;
        }

        if (reply->type == REDIS_REPLY_ERROR) {
            spdlog::warn("Heartbeat logic error: {}", reply->str);
        } else {
            // spdlog::trace("Heartbeat sent successfully."); // 仅在极详细模式下开启
        }

        freeReplyObject(reply);
    }

    long long ClusterImpl::countActiveNodes()
    {
        // 假设外部 sample 已经加锁
        if (!ensureConnection()) {
            // 这里抛出异常，通知上层 sample 流程中断
            spdlog::warn("countActiveNodes: Connection lost.");
            throw RecoverableError("Connection lost while counting nodes");
        }

        long long count = 0;
        std::string cursor = "0";
        
        do {
            redisReply* reply = (redisReply*)redisCommand(ctx_, "SCAN %s MATCH %s COUNT 100", 
                                                          cursor.c_str(), heartbeatPattern_.c_str());
            
            if (reply == nullptr) {
                isConnected_ = false; 
                if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
                spdlog::error("countActiveNodes: SCAN command failed (IO Error)");
                throw RecoverableError("IO Error during SCAN");
            }

            if (reply->type == REDIS_REPLY_ERROR) {
                std::string err = reply->str;
                freeReplyObject(reply);
                spdlog::error("countActiveNodes: SCAN command error: {}", err);
                throw RecoverableError("Redis Protocol Error during SCAN: " + err);
            }

            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
                // Update cursor
                redisReply* cursorReply = reply->element[0];
                if (cursorReply->type == REDIS_REPLY_STRING) {
                    cursor = std::string(cursorReply->str, cursorReply->len);
                }

                // Count keys
                redisReply* keysReply = reply->element[1];
                if (keysReply->type == REDIS_REPLY_ARRAY) {
                    count += keysReply->elements;
                }
            } else {
                freeReplyObject(reply);
                spdlog::error("countActiveNodes: Unexpected SCAN response format");
                throw RecoverableError("Unexpected SCAN response format");
            }

            freeReplyObject(reply);

        } while (cursor != "0");

        return count;
    }

    std::vector<ClusterMetrics> ClusterImpl::sample(std::vector<ClusterLabel>& labels)
    {
        // 全局加锁，防止心跳线程干扰
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);

        std::vector<ClusterMetrics> metricsList;
        ClusterMetrics m;

        // 1. 检查连接
        if (!ensureConnection())throw FatalError("Redis connection unavailable");
        m.redisConnected = 1.0;

        // 2. Ping 延迟测试
        auto start = std::chrono::steady_clock::now();
        redisReply* reply = (redisReply*)redisCommand(ctx_, "PING");
        auto end = std::chrono::steady_clock::now();

        if (reply == nullptr)
        {
            isConnected_ = false;
            if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
            spdlog::warn("Sampling failed: PING IO error.");
            m.redisLatencyMs=-1.0;
        }
        else if (reply->type == REDIS_REPLY_ERROR)
        {
            std::string err = reply->str;
            freeReplyObject(reply);
            spdlog::error("Sampling failed: PING protocol error: {}", err);
            m.redisLatencyMs=-1.0;
        }
        else
        {
            std::chrono::duration<double, std::milli> elapsed = end - start;
            m.redisLatencyMs = elapsed.count();
            freeReplyObject(reply);
        }

        // 3. 统计节点 (如果前面 Ping 成功，连接大概率是好的，但仍需捕获异常)
        try
        {
            m.activeNodeCount = static_cast<double>(countActiveNodes());
        }
        catch (const RecoverableError& e)
        {
            m.activeNodeCount=-1.0;
            spdlog::warn("Sampling aborted during node counting: {}", e.what());
        }
        metricsList.push_back(m);
        return metricsList;
    }
}

#endif