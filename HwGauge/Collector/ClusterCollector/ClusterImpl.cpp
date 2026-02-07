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
    ClusterImpl::ClusterImpl(const ClusterConfig& config)
        : config_(config), ctx_(nullptr), isConnected_(false), keepRunning_(false)
    {
        spdlog::info("Initializing ClusterImpl for NodeID: {} at {}:{}", config_.nodeId, config_.host, config_.port);
        
        // 构造函数中立即尝试连接
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (!ensureConnection())
        {
            spdlog::error("Fatal Error: Failed to establish initial connection to Redis at {}:{}", config_.host, config_.port);
            throw FatalError("Initial Redis connection failed");
        }
        spdlog::info("Successfully connected to Redis at startup.");

        // 根据配置自动启动心跳
        if (config_.heartbeat)startHeartbeat();
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

    // 内部连接函数
    bool ClusterImpl::ensureConnection()
    {
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (ctx_ && isConnected_) return true;

        // 清理旧连接
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
        }

        int portInt = 6379;
        try {
            portInt = std::stoi(config_.port);
        } catch (...) {
            spdlog::warn("Invalid port [{}], defaulting to 6379", config_.port);
            portInt = 6379;
        }

        struct timeval timeout = { 1, 0 }; // 连接超时 1秒
        ctx_ = redisConnectWithTimeout(config_.host.c_str(), portInt, timeout);
        
        if (ctx_ == nullptr || ctx_->err) {
            std::string errStr = (ctx_) ? ctx_->errstr : "can't allocate context";
            spdlog::warn("Redis connection failed to {}:{}. Error: {}", config_.host, portInt, errStr);
            
            if (ctx_) {
                redisFree(ctx_);
                ctx_ = nullptr;
            }
            isConnected_ = false;
            return false;
        }

        // 处理密码认证
        if (!config_.password.empty()) {
            redisReply* reply = (redisReply*)redisCommand(ctx_, "AUTH %s", config_.password.c_str());
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                std::string err = (reply) ? reply->str : "Unknown AUTH error";
                spdlog::error("Redis authentication failed: {}", err);
                if (reply) freeReplyObject(reply);
                redisFree(ctx_);
                ctx_ = nullptr;
                isConnected_ = false;
                return false;
            }
            freeReplyObject(reply);
        }
        
        // 设置读写超时 500ms
        struct timeval io_timeout = { 0, 500000 }; 
        redisSetTimeout(ctx_, io_timeout);

        isConnected_ = true;
        spdlog::debug("Redis re-connected successfully.");
        return true;
    }

    void ClusterImpl::startHeartbeat()
    {
        std::lock_guard<std::recursive_mutex> lock(redisMutex_);
        if (keepRunning_)
        {
            spdlog::warn("Heartbeat thread already running.");
            return; 
        }
        
        spdlog::info("Starting heartbeat thread for NodeID: {}, TTL: {}s", config_.nodeId, config_.ttlSeconds);
        keepRunning_ = true;
        
        // 捕获 this 指针即可访问成员 config_
        heartbeatThread_ = std::thread([this]() {
            int intervalMs = (this->config_.ttlSeconds * 1000) / 2;
            if (intervalMs < 100) intervalMs = 100;

            while (this->keepRunning_) {
                {
                    // 只在发送数据的瞬间加锁
                    std::lock_guard<std::recursive_mutex> lock(this->redisMutex_);
                    this->sendHeartbeatPayload();
                }
                
                // 睡眠等待 (检查 keepRunning 以便快速退出)
                for (int i = 0; i < 10 && this->keepRunning_; ++i) {
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

    void ClusterImpl::sendHeartbeatPayload()
    {
        // 这里不能抛出异常，否则线程会挂掉，只能记录日志并静默失败等待重试
        if (!ensureConnection()) {
            return;
        }

        std::string key = "cluster:node:" + config_.nodeId + ":heartbeat";
        // SET key 1 EX ttl
        redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d", key.c_str(), "1", config_.ttlSeconds);

        if (reply == nullptr) {
            spdlog::warn("Heartbeat failed: IO Error. Connection marked down.");
            isConnected_ = false; 
            if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
            return;
        }

        if (reply->type == REDIS_REPLY_ERROR) {
            spdlog::warn("Heartbeat logic error: {}", reply->str);
        } else {
            // spdlog::trace("Heartbeat sent successfully."); 
        }

        freeReplyObject(reply);
    }

    long long ClusterImpl::countActiveNodes()
    {
        // 假设外部 sample 已经加锁
        if (!ensureConnection()) {
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
        // 全局加锁
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
            m.redisLatencyMs = -1.0;
        }
        else if (reply->type == REDIS_REPLY_ERROR)
        {
            std::string err = reply->str;
            freeReplyObject(reply);
            spdlog::error("Sampling failed: PING protocol error: {}", err);
            m.redisLatencyMs = -1.0;
        }
        else
        {
            std::chrono::duration<double, std::milli> elapsed = end - start;
            m.redisLatencyMs = elapsed.count();
            freeReplyObject(reply);
        }

        // 3. 统计节点
        try
        {
            m.activeNodeCount = static_cast<double>(countActiveNodes());
        }
        catch (const RecoverableError& e)
        {
            m.activeNodeCount = -1.0;
            spdlog::warn("Sampling aborted during node counting: {}", e.what());
        }
        metricsList.push_back(m);
        return metricsList;
    }

}
#endif