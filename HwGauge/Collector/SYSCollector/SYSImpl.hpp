#pragma once

#ifdef __linux__

#include "SYSMetrics.hpp"
#include <vector>
#include <chrono>
#include <map>
#include <fstream>
#include <atomic>
#include <thread>
#include <mutex>

namespace hwgauge
{
    // 辅助结构：用于保存上一次的计数器状态
    struct DiskState
    {
        unsigned long long sectorsRead;
        unsigned long long sectorsWritten;
        unsigned long long timeDoingIO; // milliseconds
    };

    struct NetState
    {
        unsigned long long bytesRx;
        unsigned long long bytesTx;
    };

    enum class PowerParseType
    {
        None,
        DCMI,
        Sensor
    };

    class SYSImpl
    {
    public:
        SYSImpl();
        ~SYSImpl();

        SYSImpl(const SYSImpl&) = delete;
        SYSImpl& operator=(const SYSImpl&) = delete;
        SYSImpl(SYSImpl&&) = delete;
        SYSImpl& operator=(SYSImpl&&) = delete;

        std::string name() { return "Linux System Collector"; }

        // 获取标签（通常只有一个 "Host"）
        std::vector<SYSLabel> labels();
        
        // 采样并计算速率
        std::vector<SYSMetrics> sample(std::vector<SYSLabel>& labels);

    private:
        // 上一个时钟周期
        std::chrono::steady_clock::time_point lastTime;

        // 常驻文件流 (优化文件 I/O) 
        std::ifstream fileMem_;
        std::ifstream fileDisk_;
        std::ifstream fileNet_;
        
        // 缓存上一次的磁盘和网络计数器，用于做减法计算速率
        std::map<std::string, DiskState> lastDiskStates;
        std::map<std::string, NetState> lastNetStates;

        // --- 整机功耗获取指令类型 ---
        PowerParseType powerParseType_;
        // 存储最终选定的整机功耗获取指令
        std::string powerCmd_; 
        // 辅助函数：初始化探测整机功耗获取指令---
        void initPowerCmd(); 

        // --- 异步功耗相关 ---
        std::atomic<double> cachedPowerWatts_; // 最新的功耗值 (主线程读这个)
        std::atomic<bool> stopThread_;         // 线程停止标志
        std::thread powerThread_;              // 专用的功耗采集线程
        // 获取功耗线程函数
        void powerWorker();
        // 通过命令获取功耗函数 
        double fetchPowerFromHardware();

        // 辅助函数：判断是否为物理磁盘（避免统计 sda1 这种分区导致吞吐量双倍）---
        bool isPhysicalDisk(const std::string& name);

        // 内部读取函数
        void readMemory(SYSMetrics& m);
        void readDisk(SYSMetrics& m, double elapsedSeconds);
        void readNetwork(SYSMetrics& m, double elapsedSeconds);
        void readPower(SYSMetrics& m);
    };
}

#endif