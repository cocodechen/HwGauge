#include "PCM.hpp"
#include "cpucounters.h"

#include <stdexcept>
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
            throw std::runtime_error("PCM: Failed to get PCM instance");

        auto status = pcmInstance->program();

        switch (status) {
        case pcm::PCM::Success:
            break;

        case pcm::PCM::MSRAccessDenied:
            throw std::runtime_error(
                "PCM: Access to CPU counters denied. Run with privileges.");

        case pcm::PCM::PMUBusy:
            pcmInstance->resetPMU();
            if (pcmInstance->program() != pcm::PCM::Success)
                throw std::runtime_error("PCM: PMU busy and reset failed");
            break;

        default:
            throw std::runtime_error("PCM init failed: " + std::to_string(int(status))));
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
            throw std::runtime_error("PCM: Not initialized");

        std::vector<CPULabel> labels;
        auto numSockets = pcmInstance->getNumSockets();

        for (pcm::uint32 s = 0; s < numSockets; ++s)
            labels.push_back(CPULabel{
                .index = s,
                .name = "Socket {}" + std::to_string(s)
                });

        return labels;
    }

    /* ---------- sampling ---------- */

    std::vector<CPUMetrics> PCM::sample()
    {
        if (!initialized || !pcmInstance)
            throw std::runtime_error("PCM: Not initialized");

        snapshot(afterState);
        auto afterTime = std::chrono::steady_clock::now();

        double elapsed =
            std::chrono::duration_cast<std::chrono::duration<double>>(
                afterTime - beforeTime).count();

        std::vector<CPUMetrics> metrics;
        auto numSockets = pcmInstance->getNumSockets();

        for (pcm::uint32 s = 0; s < numSockets; ++s)
        {
            const auto& before = beforeState[s];
            const auto& after = afterState[s];

            CPUMetrics m{};

            /* ===== Frequency (Hz, averaged active) ===== */
            m.cpuFrequency =
                pcm::getActiveAverageFrequency(before, after);

            /* ===== C-state (%) ===== */
            m.c0Residency =
                100.0 * pcm::getPackageCStateResidency(0, before, after);

            m.c6Residency =
                100.0 * pcm::getPackageCStateResidency(6, before, after);

            /* ===== Power (W) ===== */
            m.powerUsage =
                pcm::getConsumedJoules(before, after) / elapsed;

            m.memoryPowerUsage =
                pcm::getDRAMConsumedJoules(before, after) / elapsed;

            /* ===== Memory BW (Bytes/s) ===== */
            double readBytes = pcm::getBytesReadFromMC(before, after);
            double writeBytes = pcm::getBytesWrittenToMC(before, after);

            m.memoryReadBandwidth = readBytes / elapsed;
            m.memoryWriteBandwidth = writeBytes / elapsed;

            metrics.push_back(m);
        }

        // IMPORTANT: avoid copy-assignment, only swap is allowed
        beforeState.swap(afterState);
        beforeTime = afterTime;

        return metrics;
    }

} // namespace hwgauge
