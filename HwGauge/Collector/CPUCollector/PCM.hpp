#pragma once
#ifdef HWGAUGE_USE_INTEL_PCM
#include "CPUMetrics.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace pcm {
    class PCM;
    class SystemCounterState;
    class SocketCounterState;
}

namespace hwgauge {

    class PCM {
    public:
        explicit PCM();
        ~PCM();

        PCM(const PCM&) = delete;
        PCM& operator=(const PCM&) = delete;

        PCM(PCM&& other) noexcept;
        PCM& operator=(PCM&& other) noexcept;

        std::string name() { return "PCM CPU Collector"; }

        std::vector<CPULabel>   labels();
        std::vector<CPUMetrics> sample(std::vector<CPULabel>&labels);

    private:
        bool initialized{ false };
        pcm::PCM* pcmInstance{ nullptr };

        std::vector<pcm::SocketCounterState> beforeState;
        std::vector<pcm::SocketCounterState> afterState;

        std::chrono::steady_clock::time_point beforeTime;

        void initializePCM();
        void cleanupPCM();

        void snapshot(std::vector<pcm::SocketCounterState>& out);
    };

} // namespace hwgauge
#endif