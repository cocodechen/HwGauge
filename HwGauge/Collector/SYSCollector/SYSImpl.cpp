#ifdef __linux__

#include "Exceptions.hpp"
#include "SYSImpl.hpp"

#include "spdlog/spdlog.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <array>
#include <thread>

namespace hwgauge
{
    SYSImpl::SYSImpl()
    {
        // 初始化时间
        lastTime = std::chrono::steady_clock::now();
        
        // 初始化基准数据 (为了避免第一次采集出现巨大的速率尖峰)
        // 我们手动构造一个 label 跑一次流程，填充 lastDiskStates 和 lastNetStates
        // 这里的日志可能会在启动时打印一次，是可以接受的
        std::vector<SystemLabel> dummy = labels();
        sample(dummy); 
    }

    std::vector<SYSLabel> SYSImpl::labels()
    {
        return { {"LocalHost"} };
    }

    std::vector<SYSMetrics> SYSImpl::sample(std::vector<SYSLabel>& labels)
    {
        std::vector<SYSMetrics> results;

        // 计算时间差
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - lastTime;
        double dt = elapsed.count();
        if (dt <= 0) dt = 0.0001;
        lastTime = now;

        // 3. 循环 Labels (保持语义正确)
        for (const auto& label : labels)
        {
            SYSMetrics m; // 构造函数已默认初始化为 -1
            // 这里的逻辑变得非常干净，错误处理下放到具体函数
            readMemory(m);
            readDisk(m, dt);
            readNetwork(m, dt);
            readPower(m);
            results.push_back(m);
        }
        return results;
    }

    // --- 内存读取 (/proc/meminfo) ---
    void SYSImpl::readMemory(SYSMetrics& m)
    {
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()){
            throw RecoverableError("[SYSImpl] Cannot open /proc/meminfo");
        }
        std::string line, key;
        long long value;
        long long total = -1, available = -1;

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            iss >> key >> value;
            if (key == "MemTotal:") total = value;
            else if (key == "MemAvailable:") available = value;
        }
        
        // /proc/meminfo 单位是 kB
        if (total > 0)
        {
            m.memTotalGB = total / 1024.0 / 1024.0;
            long long used = total - available;
            m.memUsedGB = used / 1024.0 / 1024.0;
            m.memUtilizationPercent = (double)used / total * 100.0;
        }
        else
        {
            spdlog::warn("[SYSImpl] Memory collection failed");
            m.memTotalGB = -1.0;
            m.memUsedGB = -1.0;
            m.memUtilizationPercent = -1.0;
        }
    }

     // --- 辅助函数：判断是否为物理磁盘（避免统计 sda1 这种分区导致吞吐量双倍）---
    bool isPhysicalDisk(const std::string& name)
    {
        // 1. 过滤 loop (回环), ram (内存盘), sr (光驱)
        if (name.find("loop") == 0 || name.find("ram") == 0 || name.find("sr") == 0) return false;

        // 2. 过滤虚拟设备 (dm-*)，视情况而定，LVM 通常看 dm，这里为了简单只看物理硬件
        if (name.find("dm-") == 0) return false;

        // 3. 区分 SATA/SAS 盘 (sda, sdb...) vs 分区 (sda1, sdb2...)
        if (name.find("sd") == 0 || name.find("vd") == 0) {
            // 如果最后一位是数字，通常是分区 (sda1)，如果不是数字，是盘 (sda)
            char last = name.back();
            if (isdigit(last)) return false; 
        }

        // 4. 区分 NVMe 盘 (nvme0n1) vs 分区 (nvme0n1p1)
        if (name.find("nvme") == 0) {
            // 如果包含 'p' 且 p 后面跟数字，通常是分区
            if (name.find('p') != std::string::npos) return false; 
        }
        return true;
    }

    // --- 磁盘读取 (/proc/diskstats) ---
    void SYSImpl::readDisk(SYSMetrics& m, double dt)
    {
        std::ifstream file("/proc/diskstats");
         if (!file.is_open()) {
            throw RecoverableError("[SYSImpl] Cannot open /proc/diskstats");
        }
        std::string line;
        
        double totalReadBytes = 0;
        double totalWriteBytes = 0;
        double maxUtil = 0;

        // 当前时刻的临时 map
        std::map<std::string, DiskState> currentStates;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            int major, minor;
            std::string devName;
            // diskstats 格式复杂，通常第3列是设备名，后面跟着一堆统计
            iss >> major >> minor >> devName;

            // 过滤非物理设备
            if (!isPhysicalDisk(devName)) continue;
            
            unsigned long long r_ios, r_merges, r_sectors, r_ticks;
            unsigned long long w_ios, w_merges, w_sectors, w_ticks;
            unsigned long long io_in_progress, time_io, time_io_weighted;

            // 读取后续字段 (Kernel 4.18+)
            iss >> r_ios >> r_merges >> r_sectors >> r_ticks 
                >> w_ios >> w_merges >> w_sectors >> w_ticks 
                >> io_in_progress >> time_io >> time_io_weighted;

            DiskState ds = { r_sectors, w_sectors, time_io };
            currentStates[devName] = ds;

            // 如果上一次有记录，计算差值
            if (lastDiskStates.count(devName)) {
                auto& old = lastDiskStates[devName];
                
                // 吞吐量 (扇区 * 512字节)
                double r_diff = (double)(ds.sectorsRead - old.sectorsRead) * 512.0;
                double w_diff = (double)(ds.sectorsWritten - old.sectorsWritten) * 512.0;
                
                totalReadBytes += r_diff;
                totalWriteBytes += w_diff;

                // 利用率 (time_io 是毫秒)
                double util_diff = (double)(ds.timeDoingIO - old.timeDoingIO);
                // 比如经过 1秒(1000ms)，IO耗时 500ms，则利用率为 50%
                double util = (util_diff / (dt * 1000.0)) * 100.0;
                if (util > maxUtil) maxUtil = util;
            }
        }

        lastDiskStates = currentStates; // 更新状态

        m.diskReadMBps = totalReadBytes / 1024.0 / 1024.0 / dt;
        m.diskWriteMBps = totalWriteBytes / 1024.0 / 1024.0 / dt;
        m.maxDiskUtilPercent = (maxUtil > 100.0) ? 100.0 : maxUtil; // 修正多线程可能导致的>100%
    }

    // --- 网络读取 (/proc/net/dev) ---
    void SYSImpl::readNetwork(SYSMetrics& m, double dt)
    {
        std::ifstream file("/proc/net/dev");
        if (!file.is_open()) {
            throw RecoverableError("[SYSImpl] Cannot open /proc/net/dev");
        }
        std::string line;
        
        double totalRx = 0;
        double totalTx = 0;
        std::map<std::string, NetState> currentStates;

        // 跳过前两行表头
        std::getline(file, line);
        std::getline(file, line);

        while (std::getline(file, line)) {
            // 处理格式: "  eth0: 1234 56 ..."
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string devName = line.substr(0, colonPos);
            // 去除空格
            devName.erase(0, devName.find_first_not_of(" "));

            // 过滤回环 lo
            if (devName == "lo") continue;

            std::string statsPart = line.substr(colonPos + 1);
            std::istringstream iss(statsPart);
            
            unsigned long long rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCom, rxMulti;
            unsigned long long txBytes, txPackets, txErrs, txDrop, txFifo, txColl, txCarrier, txCom;

            iss >> rxBytes >> rxPackets >> rxErrs >> rxDrop >> rxFifo >> rxFrame >> rxCom >> rxMulti
                >> txBytes >> txPackets >> txErrs >> txDrop >> txFifo >> txColl >> txCarrier >> txCom;

            NetState ns = { rxBytes, txBytes };
            currentStates[devName] = ns;

            if (lastNetStates.count(devName)) {
                auto& old = lastNetStates[devName];
                totalRx += (double)(ns.bytesRx - old.bytesRx);
                totalTx += (double)(ns.bytesTx - old.bytesTx);
            }
        }

        lastNetStates = currentStates;

        m.netDownloadMBps = totalRx / 1024.0 / 1024.0 / dt;
        m.netUploadMBps = totalTx / 1024.0 / 1024.0 / dt;
    }

    // --- 功耗读取 (IPMI 调用) ---
    void SYSImpl::readPower(SYSMetrics& m)
    {
        // 注意：这需要配置 sudo 免密，或者当前用户是 root
        // 且需要安装 ipmitool
        // 命令：sudo ipmitool dcmi power reading
        
        std::array<char, 128> buffer;
        std::string result;`
        // 使用 popen 执行命令
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("ipmitool dcmi power reading 2>/dev/null", "r"), pclose);
        
        if (!pipe) {
            m.systemPowerWatts = -1.0; // 失败标记
            return;
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        // 检查结果中是否包含权限错误
        if (result.find("Permission denied") != std::string::npos || 
            result.find("password") != std::string::npos){
            throw std::RecoverableError("Permission denied (sudo required)");
        }
        
        if (result.find("Could not open device") != std::string::npos) {
             throw std::RecoverableError("Cannot access IPMI device (driver not loaded?)");
        }

        // 解析: "Instantaneous power reading: 145 Watts"
        std::string key = "Instantaneous power reading:";
        size_t pos = result.find(key);
        if (pos != std::string::npos) {
            // 找到数字位置
            size_t numStart = pos + key.length();
            try {
                // 简单解析后面的数字
                m.systemPowerWatts = std::stod(result.substr(numStart));
            } catch (...) {
                m.systemPowerWatts = 0.0;
            }
        } else {
            m.systemPowerWatts = -1.0; // 未找到，可能机器不支持
        }
    }
}

#endif