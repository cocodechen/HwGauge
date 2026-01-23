#ifdef __linux__

#include "Collector/Exception.hpp"
#include "SYSImpl.hpp"

#include "spdlog/spdlog.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <array>
#include <sys/time.h> 
#include <sys/resource.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace hwgauge
{
    SYSImpl::SYSImpl(): cachedPowerWatts_(-1.0), stopThread_(false)
    {
        // 初始化时间
        lastTime = std::chrono::steady_clock::now();

        // --- 优化 : 预先打开文件 ---
        // /proc 文件一旦打开，其实是指向了内核的一个 handle。
        // 即使内容变了，只要不关闭，用 seekg(0) 就能重读最新数据。
        fileMem_.open("/proc/meminfo");
        fileDisk_.open("/proc/diskstats");
        fileNet_.open("/proc/net/dev");

        if (!fileMem_ || !fileDisk_ || !fileNet_)throw hwgauge::FatalError("[SYSImpl] Failed to keep open /proc files.");

        // --- 核心：初始化探测命令 ---
        initPowerCmd();
        
        // 初始化基准数据 (为了避免第一次采集出现巨大的速率尖峰)
        // 我们手动构造一个 label 跑一次流程，填充 lastDiskStates 和 lastNetStates
        // 这里的日志可能会在启动时打印一次，是可以接受的
        std::vector<SYSLabel> dummy = labels();
        sample(dummy); 

        // 启动后台线程
        powerThread_ = std::thread(&SYSImpl::powerWorker, this);
    }

    SYSImpl::~SYSImpl()
    {
        // 关闭文件流
        if (fileMem_.is_open()) fileMem_.close();
        if (fileDisk_.is_open()) fileDisk_.close();
        if (fileNet_.is_open()) fileNet_.close();
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

        // 循环 Labels (保持语义正确)
        for (const auto& label : labels)
        {
            SYSMetrics m; // 构造函数已默认初始化为 -1
            readMemory(m);
            readDisk(m, dt);
            readNetwork(m, dt);
            readPower(m);
            results.push_back(m);
        }
        return results;
    }

    // --- 功耗命令探测函数 ---
    void SYSImpl::initPowerCmd()
    {
        spdlog::info("[SYSImpl] Detecting power monitoring command...");
        // 1. 优先尝试 DCMI (标准命令)
        std::string cmd = "sudo nice -n -10 ipmitool dcmi power reading 2>&1";
        spdlog::info("[SYSImpl] Detecting power by using cmd :{}",cmd);
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (pipe)
        {
            std::array<char, 256> buffer;
            std::string output;
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                output += buffer.data();
            }
            // 检查是否包含关键字
            if (output.find("Instantaneous power reading") != std::string::npos && 
                output.find("Watts") != std::string::npos)
            {
                powerCmd_ = cmd;
                powerParseType_ = PowerParseType::DCMI;
                spdlog::info("[SYSImpl] Selected DCMI command: {}", powerCmd_);
                return; // 找到即停止
            }
        }

        // 2. 尝试传感器列表 (使用 -c CSV 格式加速解析)
        const std::vector<std::string> sensorNames = {
            "POWER_USAGE",      // Cisco
            "Pwr Consumption",  // Dell
            "System Power",     // Supermicro
            "System Level",     // HP
            "Total Power"       // Lenovo
        };
        for (const auto& name : sensorNames)
        {
            // 构造 CSV 查询命令
            std::string cmd = "sudo nice -n -10 ipmitool sensor reading \"" + name + "\" -c 2>&1";
            spdlog::info("[SYSImpl] Detecting power by using cmd :{}",cmd);
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
            if (pipe)
            {
                std::array<char, 256> buffer;
                if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
                {
                    std::string output = buffer.data();
                    // CSV 格式通常是: Name,Value,Watts,...
                    auto pos = output.find(',');
                    if (pos == std::string::npos)continue;
                    std::string value = output.substr(pos + 1);
                    // 去掉换行
                    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
                    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
                    //排除na
                    if (value == "na" || value.empty())continue;
                    //排除非数字
                    char* end = nullptr;
                    std::strtod(value.c_str(), &end);
                    if (end == value.c_str() || *end != '\0')continue;

                    powerParseType_ = PowerParseType::Sensor;
                    powerCmd_ = cmd;
                    spdlog::info("[SYSImpl] Selected Sensor command: {}", powerCmd_);
                    return; // 找到即停止
                }
            }
        }
        powerCmd_="";
        powerParseType_ = PowerParseType::None;
        // 3. 都失败了
        throw FatalError("[SYSImpl] No supported power monitoring method found.");
    }

    // --- 判断是否为物理磁盘---
    bool SYSImpl::isPhysicalDisk(const std::string& name)
    {
        // 1. 过滤 loop (回环), ram (内存盘), sr (光驱)
        if (name.find("loop") == 0 || name.find("ram") == 0 || name.find("sr") == 0) return false;
        // 2. 过滤虚拟设备 (dm-*)，视情况而定，LVM 通常看 dm，这里为了简单只看物理硬件
        if (name.find("dm-") == 0) return false;
        // 3. 区分 SATA/SAS 盘 (sda, sdb...) vs 分区 (sda1, sdb2...)
        if (name.find("sd") == 0 || name.find("vd") == 0)
        {
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

    // --- 内存读取 (/proc/meminfo) ---
    void SYSImpl::readMemory(SYSMetrics& m)
    {
        if (!fileMem_.is_open())throw FatalError("[SYSImpl] Cannot open /proc/meminfo");
        // 重置文件流状态和指针
        fileMem_.clear();
        fileMem_.seekg(0);

        std::string line, key;
        long long value;
        long long total = -1, available = -1;

        while (std::getline(fileMem_, line)) {
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

    // --- 磁盘读取 (/proc/diskstats) ---
    void SYSImpl::readDisk(SYSMetrics& m, double dt)
    {
         if (!fileDisk_.is_open())throw FatalError("[SYSImpl] Cannot open /proc/diskstats");
        fileDisk_.clear();
        fileDisk_.seekg(0);

        std::string line;
        double totalReadBytes = 0;
        double totalWriteBytes = 0;
        double maxUtil = 0;

        // 当前时刻的临时 map
        std::map<std::string, DiskState> currentStates;

        while (std::getline(fileDisk_, line))
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
        if (!fileNet_.is_open())throw FatalError("[SYSImpl] Cannot open /proc/net/dev");
        fileNet_.clear();
        fileNet_.seekg(0);

        std::string line;
        double totalRx = 0;
        double totalTx = 0;
        std::map<std::string, NetState> currentStates;

        // 跳过前两行表头
        std::getline(fileNet_, line);
        std::getline(fileNet_, line);

        while (std::getline(fileNet_, line)) {
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

    // --- 主线程调用的功耗读取函数 (极速) ---
    void SYSImpl::readPower(SYSMetrics& m)
    {
        // 主线程只做一件事：读原子变量
        // 无论系统负载多高，这里永不卡顿
        m.systemPowerWatts = cachedPowerWatts_.load();
    }

    // --- 后台线程函数 (即使卡顿也没关系) ---
    void SYSImpl::powerWorker()
    {
        // 设置高优先级: 简单 nice (推荐)
        // 既然整个程序已经是 sudo 运行的，直接设 nice 即可
        // 0 是默认，-20 是最高。设个 -10 足够抢占 CPU 了。
        id_t tid = syscall(SYS_gettid); 
        setpriority(PRIO_PROCESS, tid, -10); 
        while (!stopThread_)
        {
            // 1. 执行耗时的硬件采集
            double watts = fetchPowerFromHardware();
            // 2. 如果采集成功，更新缓存
            if (watts > 0) {
                cachedPowerWatts_.store(watts);
            }
            // 3. 休眠等待下一次采集
            // 使用小步休眠，以便能及时响应 stopThread_
            // 假设我们希望功耗数据每 5 秒刷新一次
            for (int i = 0; i < 50; ++i) { // 50 * 100ms = 5s
                if (stopThread_) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    // --- 功耗读取函数 ---
    double SYSImpl::fetchPowerFromHardware()
    {
        if (powerCmd_.empty())throw FatalError("[SYSImpl] No supported power monitoring method found.");
        try
        {
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(powerCmd_.c_str(), "r"), pclose);
            if (!pipe)
            {
                spdlog::warn("[SYSImpl] Failed to get machine power");
                return -1.0;
            }
            std::array<char, 256> buffer;
            std::string result;
            // DCMI 可能输出多行，Sensor 输出单行。读取所有输出比较稳妥。
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            // --- 根据探测阶段确定的策略进行解析 ---
            if (powerParseType_ == PowerParseType::DCMI)
            {
                // 解析: "Instantaneous power reading: 123 Watts"
                std::string key = "Instantaneous power reading:";
                size_t pos = result.find(key);
                if (pos != std::string::npos)
                {
                    // 1. 截取关键字之后的所有内容
                    std::string sub = result.substr(pos + key.length());
                    
                    // 2. 使用 stringstream 自动跳过空格并解析出第一个数值
                    std::stringstream ss(sub);
                    double res = -1.0;
                    if (ss >> res)return res;
                }
            }
            else if (powerParseType_ == PowerParseType::Sensor)
            {
                // 解析 CSV: "Name,123,Watts,OK" 或 "Name,123"
                size_t firstComma = result.find(',');
                if (firstComma != std::string::npos)
                {
                    std::string numStr;
                    // 找第二个逗号
                    size_t secondComma = result.find(',', firstComma + 1);
                    if (secondComma != std::string::npos){
                        // 情况 A: 标准格式 "Name,123,..." -> 取中间
                        numStr = result.substr(firstComma + 1, secondComma - firstComma - 1);
                    }else{
                        // 情况 B: 短格式 "Name,123" -> 取到末尾
                        numStr = result.substr(firstComma + 1);
                    }
                    // 关键步骤：去除可能的换行符 (\n) 和首尾空格
                    // 因为 fgets 读进来的 line 可能末尾带 \n
                    numStr.erase(0, numStr.find_first_not_of(" \t\n\r"));
                    numStr.erase(numStr.find_last_not_of(" \t\n\r") + 1);
                    if (!numStr.empty())
                    {
                        //std::cout << "Parsed Power String: [" << numStr << "]" << std::endl; // 调试用
                        double res = std::stod(numStr);
                        return res;
                    }
                }
            }
        } catch (...) {
            // 忽略读取过程中的异常
        }
        spdlog::warn("[SYSImpl] Failed to get machine power");
        return -1.0;
    }
}

#endif