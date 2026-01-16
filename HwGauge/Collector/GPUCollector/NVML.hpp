#pragma once

#ifdef HWGAUGE_USE_NVML

#include "GPUMetrics.hpp"
#include <string>
#include <vector>

namespace hwgauge {
	class NVML {
	public:
		explicit NVML();
		~NVML();

		NVML(const NVML&) = delete;
		NVML& operator=(const NVML&) = delete;

		NVML(NVML&& other) noexcept;
		NVML& operator=(NVML&& other) noexcept;

		std::string name() { return "NVML Collector"; }
		std::vector<GPULabel> labels();
		std::vector<GPUMetrics> sample(std::vector<GPULabel>&labels);

	private:
		bool initialized = false;
	};
}
#endif