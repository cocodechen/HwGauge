#include "PCM.hpp"
#include "cpucounters.h"
#include <format>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace hwgauge {

	PCM::PCM() : initialized(false), pcmInstance(nullptr)
	{
		initializePCM();
	}

	PCM::~PCM()
	{
		if (initialized) {
			cleanupPCM();
		}
	}

	PCM::PCM(PCM&& other) noexcept
		: initialized(other.initialized),
		pcmInstance(other.pcmInstance),
		beforeState(std::move(other.beforeState)),
		afterState(std::move(other.afterState))
	{
		other.initialized = false;
		other.pcmInstance = nullptr;
	}

	PCM& PCM::operator=(PCM&& other) noexcept
	{
		if (this != &other)
		{
			if (initialized)
			{
				cleanupPCM();
			}

			initialized = other.initialized;
			pcmInstance = other.pcmInstance;
			beforeState = std::move(other.beforeState);
			afterState = std::move(other.afterState);

			other.initialized = false;
			other.pcmInstance = nullptr;
		}

		return *this;
	}

	void PCM::initializePCM()
	{
		pcmInstance = pcm::PCM::getInstance();

		if (!pcmInstance) {
			throw std::runtime_error("PCM: Failed to get PCM instance");
		}

		// Program PCM counters
		pcm::PCM::ErrorCode status = pcmInstance->program();

		switch (status) {
		case pcm::PCM::Success:
			break;
		case pcm::PCM::MSRAccessDenied:
			throw std::runtime_error("PCM: Access to CPU counters denied. Try running with elevated privileges.");
		case pcm::PCM::PMUBusy:
			// Try to reset and reprogram
			pcmInstance->resetPMU();
			status = pcmInstance->program();
			if (status != pcm::PCM::Success) {
				throw std::runtime_error("PCM: PMU busy and reset failed");
			}
			break;
		default:
			throw std::runtime_error(std::format("PCM: Initialization failed with error code: {}", static_cast<int>(status)));
		}

		initialized = true;

		// Initialize beforeState with current counters
		uint32 numSockets = pcmInstance->getNumSockets();
		beforeState.reserve(numSockets);
		afterState.reserve(numSockets);

		for (uint32 i = 0; i < numSockets; ++i) {
			beforeState.push_back(pcmInstance->getSocketCounterState(i));
		}
	}

	void PCM::cleanupPCM()
	{
		if (pcmInstance) {
			pcmInstance->cleanup();
			pcmInstance = nullptr;
		}
		initialized = false;
	}

	std::vector<CPULabel> PCM::labels()
	{
		if (!initialized || !pcmInstance) {
			throw std::runtime_error("PCM: Not initialized");
		}

		std::vector<CPULabel> labels;
		uint32 numSockets = pcmInstance->getNumSockets();

		for (uint32 socketId = 0; socketId < numSockets; ++socketId) {
			CPULabel label = {
				.index = socketId,
				.name = std::format("Socket {}", socketId)
			};
			labels.push_back(label);
		}

		return labels;
	}

	std::vector<CPUMetrics> PCM::sample()
	{
		if (!initialized || !pcmInstance) {
			throw std::runtime_error("PCM: Not initialized");
		}

		std::vector<CPUMetrics> metrics;
		uint32 numSockets = pcmInstance->getNumSockets();

		// Update afterState with current counters
		afterState.clear();
		for (uint32 i = 0; i < numSockets; ++i) {
			afterState.push_back(pcmInstance->getSocketCounterState(i));
		}

		// Calculate metrics for each socket
		for (uint32 socketId = 0; socketId < numSockets; ++socketId) {
			const auto& before = beforeState[socketId];
			const auto& after = afterState[socketId];

			CPUMetrics m;

			// CPU Frequency (MHz)
			// Get average frequency across all cores in the socket
			m.cpuFrequency = getActiveAverageFrequency(before, after);

			// C-State Residencies (%)
			// C0 is the active state, C6 is a deep sleep state
			m.c0Residency = getCoreCStateResidency(0, before, after) * 100.0;
			m.c6Residency = getCoreCStateResidency(6, before, after) * 100.0;

			// CPU Power Usage (Watts)
			// Package power consumption
			m.powerUsage = getConsumedJoules(before, after);

			// Memory Bandwidth (MB/s)
			// DRAM read and write bandwidth
			double readBytes = getBytesReadFromMC(before, after);
			double writeBytes = getBytesWrittenToMC(before, after);

			// Convert from bytes to MB/s
			// The bandwidth functions return bytes per time interval
			m.memoryReadBandwidth = readBytes / (1024.0 * 1024.0);
			m.memoryWriteBandwidth = writeBytes / (1024.0 * 1024.0);

			// Memory Power Usage (Watts)
			// DRAM power consumption
			m.memoryPowerUsage = getDRAMConsumedJoules(before, after);

			metrics.push_back(m);
		}

		// Update beforeState for next sample
		beforeState = afterState;

		return metrics;
	}

} // namespace hwgauge