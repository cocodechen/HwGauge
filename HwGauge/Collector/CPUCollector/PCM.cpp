#ifdef HWGAUGE_USE_INTEL_PCM
#include "PCM.hpp"
#include "Collector/Exception.hpp"

#include "cpucounters.h"
#include <thread>
#include <chrono>

namespace hwgauge {

    PCM::PCM() {
        initializePCM();
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
        beforeTime(other.beforeTime)
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
    }

    void PCM::cleanupPCM()
    {
        if (pcmInstance)
            pcmInstance->cleanup();

        pcmInstance = nullptr;
        initialized = false;
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

            try
            {
                unsigned int tjmax = pcmInstance->getTjMax(s); // 或者 pcmInstance->GetTjMax(s);
                unsigned int headroom = pcmInstance->getThermalHeadroom(s); // 或者 pcmInstance->GetThermalHeadroom(s);
                
                if (tjmax > 0 && headroom < tjmax) { // 确保 headroom 是有效的
                    m.temperature = static_cast<double>(tjmax - headroom);
                } else {
                    // 如果 TjMax 或 headroom 获取失败，或者 headroom 无效，记录一个警告
                    spdlog::warn("[PCM] Could not get valid temperature for socket {}. TjMax: {}, Headroom: {}", s, tjmax, headroom);
                    m.temperature = -1.0; // 标记为未知
                }
            } catch (const std::exception& e) {
                // 如果 PCM 库抛出异常（比如 MSR 读写失败）
                spdlog::warn("[PCM] Exception getting temperature for socket {}: {}", s, e.what());
                m.temperature = -1.0; // 标记为未知
            }

            //m.temperature = (double)pcmInstance->getTemperature(s);

            metrics.push_back(m);
        }

        beforeState.swap(afterState);
        beforeTime = afterTime;

        return metrics;
    }

} // namespace hwgauge
#endif