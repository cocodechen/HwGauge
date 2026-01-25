#ifdef HWGAUGE_USE_INTEL_PCM
#include "PCM.hpp"
#include "Collector/Exception.hpp"

#include "spdlog/spdlog.h"
#include "cpucounters.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem> // C++17
#include <fstream>
#include <string>

namespace hwgauge
{
    namespace fs = std::filesystem;

    PCM::PCM() {
        initializePCM();
        initTempSensors();
    }

    PCM::~PCM() {
        if (initialized)
            cleanupPCM();
    }

    PCM::PCM(PCM&& other) noexcept
        : initialized(other.initialized),
        pcmInstance(other.pcmInstance),
        beforeState(std::move(other.beforeState)),
        afterState(std::move(other.afterState)),
        beforeTime(other.beforeTime),
        socketPaths(std::move(other.socketPaths)) // <--- 必须加上这一行！
    {
        other.initialized = false;
        other.pcmInstance = nullptr;
    }

    PCM& PCM::operator=(PCM&& other) noexcept {
        if (this != &other) {
            if (initialized)
                cleanupPCM();

            initialized = other.initialized;
            pcmInstance = other.pcmInstance;
            beforeState = std::move(other.beforeState);
            afterState = std::move(other.afterState);
            beforeTime = other.beforeTime;
            socketPaths = std::move(other.socketPaths); // <--- 必须加上这一行！

            other.initialized = false;
            other.pcmInstance = nullptr;
        }
        return *this;
    }

    /* ---------- helpers ---------- */

    void PCM::snapshot(std::vector<pcm::SocketCounterState>& out)
    {
        out.clear();
        out.reserve(pcmInstance->getNumSockets());

        for (pcm::uint32 s = 0; s < pcmInstance->getNumSockets(); ++s)
            out.push_back(pcmInstance->getSocketCounterState(s));
    }

    /* ---------- init ---------- */

    void PCM::initializePCM()
    {
        pcmInstance = pcm::PCM::getInstance();
        if (!pcmInstance)
            throw hwgauge::FatalError("PCM: Failed to get PCM instance");

        auto status = pcmInstance->program();

        switch (status) {
        case pcm::PCM::Success:
            break;

        case pcm::PCM::MSRAccessDenied:
            throw hwgauge::FatalError(
                "PCM: Access to CPU counters denied. Run with privileges.");

        case pcm::PCM::PMUBusy:
            pcmInstance->resetPMU();
            if (pcmInstance->program() != pcm::PCM::Success)
                throw hwgauge::FatalError("PCM: PMU busy and reset failed");
            break;

        default:
            throw hwgauge::FatalError("PCM init failed: " + std::to_string(int(status)));
        }

        initialized = true;

        // Prime baseline snapshot
        snapshot(beforeState);
        beforeTime = std::chrono::steady_clock::now();

        zeroBandwidthCounter=0;
    }

    void PCM::cleanupPCM()
    {
        if (pcmInstance)
            pcmInstance->cleanup();

        pcmInstance = nullptr;
        initialized = false;
    }

    // 辅助函数：尝试重置 PCM
    void PCM::resetPCM()
    {
        spdlog::warn("[PCM] Detected hung PMU counters (Bandwidth 0), attempting reset...");
        if (pcmInstance) {
            pcmInstance->cleanup();
            // 稍微等待一下让硬件状态稳定
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto status = pcmInstance->program();
            if (status != pcm::PCM::Success) {
                spdlog::error("[PCM] Reset failed with status: {}", static_cast<int>(status));
            } else {
                // 重置成功后，必须重新校准 beforeState，否则下一次计算会因为计数器归零产生巨大的错误尖峰
                snapshot(beforeState); 
                beforeTime = std::chrono::steady_clock::now();
                zeroBandwidthCounter = 0;
                spdlog::info("[PCM] Reset successful.");
            }
        }
    }

    void PCM::initTempSensors()
    {
        // 遍历所有 hwmon 目录
        if (!fs::exists("/sys/class/hwmon")) {
            spdlog::warn("[PCM] /sys/class/hwmon does not exist. Temperature unavailable.");
            return;
        }

        for (const auto& entry : fs::directory_iterator("/sys/class/hwmon")) {
            if (!entry.is_directory()) continue;
            
            // 遍历该 hwmon 目录下的所有 label 文件
            for (const auto& f : fs::directory_iterator(entry.path())) {
                std::string fname = f.path().filename().string();
                
                // 寻找 temp*_label 文件
                if (fname.find("temp") == 0 && fname.find("_label") != std::string::npos) {
                    
                    // 读取 label 内容，期望找到 "Package id X"
                    std::ifstream ifs(f.path());
                    std::string labelContent;
                    if (std::getline(ifs, labelContent)) {
                        // 典型内容: "Package id 0"
                        size_t pkgPos = labelContent.find("Package id");
                        if (pkgPos != std::string::npos) {
                            try {
                                // 提取 ID 数字
                                // "Package id " 长度是 11
                                // 但为了稳健，我们找最后一个空格
                                size_t lastSpace = labelContent.find_last_of(' ');
                                std::string idStr = labelContent.substr(lastSpace + 1);
                                uint32_t socketId = std::stoul(idStr);
                                
                                // 构造对应的 input 文件路径 (把 _label 换成 _input)
                                // 例如: temp1_label -> temp1_input
                                std::string inputPath = f.path().string();
                                size_t labelIdx = inputPath.find("_label");
                                if (labelIdx != std::string::npos) {
                                    inputPath.replace(labelIdx, 6, "_input");
                                    
                                    socketPaths[socketId] = inputPath;
                                    spdlog::info("[PCM] Mapped Socket {} temperature to {}", socketId, inputPath);
                                }
                            } catch (const std::exception& e) {
                                spdlog::warn("[PCM] Failed to parse socket ID from label: '{}'", labelContent);
                            }
                        }
                    }
                }
            }
        }
    }

    double PCM::readTemp(uint32_t socketId)
    {
        if (socketPaths.count(socketId)) {
            std::ifstream ifs(socketPaths[socketId]);
            if (ifs.is_open()) {
                long tempMilli = 0;
                if (ifs >> tempMilli) {
                    return static_cast<double>(tempMilli) / 1000.0;
                }
            }
        }
        return -1.0; // 读取失败或未找到文件
    }

    /* ---------- labels ---------- */

    std::vector<CPULabel> PCM::labels()
    {
        if (!initialized || !pcmInstance)
            throw hwgauge::FatalError("PCM: Not initialized");

        std::vector<CPULabel> labels;
        auto numSockets = pcmInstance->getNumSockets();

        for (pcm::uint32 s = 0; s < numSockets; ++s)
            labels.push_back(CPULabel{
                s,
                pcmInstance->getCPUBrandString(),
                });

        return labels;
    }

    /* ---------- sampling ---------- */

    std::vector<CPUMetrics> PCM::sample(std::vector<CPULabel>&labels)
    {
        if (!initialized || !pcmInstance)
            throw hwgauge::FatalError("PCM: Not initialized");

        snapshot(afterState);
        auto afterTime = std::chrono::steady_clock::now();

        double elapsed =
            std::chrono::duration_cast<std::chrono::duration<double>>(
                afterTime - beforeTime).count();

        std::vector<CPUMetrics> metrics;
        //auto numSockets = pcmInstance->getNumSockets();
        // 用于检测当前系统是否活跃
        bool isSystemActive = false; 
        // 用于检测是否所有 Socket 的带宽都为 0
        bool allBandwidthZero = true; 

         for (const auto& label : labels)
        {
            auto s = label.index; 
            const auto& before = beforeState[s];
            const auto& after = afterState[s];

            CPUMetrics m{};

            m.cpuUtilization = 100.0 * pcm::getExecUsage(before, after);

            m.cpuFrequency =
                pcm::getActiveAverageFrequency(before, after) / 1e6;  // Hz -> MHz

            m.c0Residency =
                100.0 * pcm::getPackageCStateResidency(0, before, after);

            m.c6Residency =
                100.0 * pcm::getPackageCStateResidency(6, before, after);

            m.powerUsage =
                pcm::getConsumedJoules(before, after) / elapsed;

            m.memoryPowerUsage =
                pcm::getDRAMConsumedJoules(before, after) / elapsed;

            double readBytes = pcm::getBytesReadFromMC(before, after);
            double writeBytes = pcm::getBytesWrittenToMC(before, after);

            m.memoryReadBandwidth = readBytes / 1e6 / elapsed;  // B/s -> MB/s
            m.memoryWriteBandwidth = writeBytes / 1e6 / elapsed;  // B/s -> MB/s 

            m.temperature = readTemp(s);

            //m.temperature = (double)pcmInstance->getTemperature(s);

            metrics.push_back(m);

            // 如果 CPU 利用率 > 1%，认为系统是活的
            if (m.cpuUtilization > 1.0) isSystemActive = true; 
            // 检查带宽数据
            if (readBytes > 0.0 || writeBytes > 0.0)allBandwidthZero = false;
        }

        // --- 自愈逻辑开始 ---
        // 如果 CPU 在工作 (利用率 > 1%)，但所有 Socket 的内存读写全都是 0，说明 PMU 可能挂了
        if (isSystemActive && allBandwidthZero)
        {
            zeroBandwidthCounter++;
            if (zeroBandwidthCounter >= 50)
            {
                resetPCM();
                return metrics; 
            }
        }
        else
        {
            // 如果数据正常，计数器归零
            zeroBandwidthCounter = 0;
        }

        beforeState.swap(afterState);
        beforeTime = afterTime;

        return metrics;
    }

} // namespace hwgauge
#endif