#pragma once

#include "CPUCollector.hpp"
#include <string>
#include <vector>
#include <memory>

// Forward declarations for PCM types
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
		std::vector<CPULabel> labels();
		std::vector<CPUMetrics> sample();

	private:
		bool initialized;
		pcm::PCM* pcmInstance;

		// Store previous counter states for calculating deltas
		std::vector<pcm::SocketCounterState> beforeState;
		std::vector<pcm::SocketCounterState> afterState;

		// Helper function to initialize PCM
		void initializePCM();

		// Helper function to cleanup PCM
		void cleanupPCM();
	};
}