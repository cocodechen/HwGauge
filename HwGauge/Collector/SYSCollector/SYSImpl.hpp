#pragma once

#ifdef __linux__

#include "SYSMetrics.hpp"
#include <vector>
#include <chrono>
#include <map>

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

    class SYSImpl
    {
    public:
        SYSImpl();
        ~SYSImpl() = default;

        std::string name() { return "Linux System Collector"; }

        // 获取标签（通常只有一个 "Host"）
        std::vector<SYSLabel> labels();
        
        // 采样并计算速率
        std::vector<SYSMetrics> sample(std::vector<SYSLabel>& labels);

    private:
        std::chrono::steady_clock::time_point lastTime;
        
        // 缓存上一次的磁盘和网络计数器，用于做减法计算速率
        std::map<std::string, DiskState> lastDiskStates;
        std::map<std::string, NetState> lastNetStates;

        // 内部读取函数
        void readMemory(SYSMetrics& m);
        void readDisk(SYSMetrics& m, double elapsedSeconds);
        void readNetwork(SYSMetrics& m, double elapsedSeconds);
        void readPower(SYSMetrics& m);
    };
}

#endif