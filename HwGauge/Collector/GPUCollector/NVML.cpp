#ifdef HWGAUGE_USE_NVML

#include "NVML.hpp"
#include <nvml.h>
#include <array>
#include "Collector/Exception.hpp"

namespace hwgauge {
	NVML::NVML()
	{
		nvmlReturn_t status = nvmlInit();
		if (status != NVML_SUCCESS) {
			throw hwgauge::FatalError("NVML initialization failed");
		}
	}

	NVML::~NVML() {
		if (initialized) {
			nvmlReturn_t status = nvmlShutdown();
			if (status != NVML_SUCCESS) {
				throw hwgauge::FatalError("NVML shutdown failed");
			}
		}
	}

	NVML::NVML(NVML&& other) noexcept
	{
		initialized = other.initialized;
		other.initialized = false;
	}

	NVML& NVML::operator=(NVML&& other) noexcept
	{
		if (this != &other)
		{
			if (initialized)
			{
				nvmlShutdown();
			}

			initialized = other.initialized;
			other.initialized = false;
		}

		return *this;
	}

	std::vector<GPULabel> NVML::labels()
	{
		nvmlReturn_t status;

		unsigned int devicesCount = 0;
		status = nvmlDeviceGetCount(std::addressof(devicesCount));

		if (status != NVML_SUCCESS) {
			throw hwgauge::FatalError("NVML get devices count failed");
		}

		std::vector<GPULabel> labels;
		for (std::size_t index = 0; index < devicesCount; index++) {
			nvmlDevice_t handle;
			status = nvmlDeviceGetHandleByIndex(index, std::addressof(handle));
			if (status != NVML_SUCCESS) {
				throw hwgauge::FatalError("NVML get devices handle failed");
			}

			std::array<char, NVML_DEVICE_NAME_BUFFER_SIZE> name = { 0 };
			status = nvmlDeviceGetName(handle, name.data(), name.size() - 1);
			if (status != NVML_SUCCESS) {
				throw hwgauge::FatalError("NVML get devices name failed");
			}

			GPULabel label = {
				index,
				std::string(name.data())
			};

			labels.emplace_back(std::move(label));
		}

		return labels;
	}

	std::vector<GPUMetrics> NVML::sample(std::vector<GPULabel>&labels)
	{
		nvmlReturn_t status;
		// status = nvmlDeviceGetCount(std::addressof(devicesCount));
		// if (status != NVML_SUCCESS) {
		// 	throw std::runtime_error("NVML get devices count failed");
		// }
		std::vector<GPUMetrics> metrics;
		for (const auto& label : labels)
		{
			auto index = label.index;   // 关键：由 label 决定设备
			nvmlDevice_t handle;
			status = nvmlDeviceGetHandleByIndex(index, std::addressof(handle));
			if (status != NVML_SUCCESS) {
				throw hwgauge::RecoverableError("NVML get devices handle failed: {}");
			}

			// GPU / Memory Utilization
			nvmlUtilization_t utilization;
			status = nvmlDeviceGetUtilizationRates(handle, std::addressof(utilization));
			if (status != NVML_SUCCESS) {
				throw hwgauge::RecoverableError("NVML get utilizations rates failed");
			}
			double gpuUtilization = utilization.gpu;
			double memoryUtilization = utilization.memory;

			// GPU Frequency
			unsigned int smClock;
			status = nvmlDeviceGetClockInfo(handle, NVML_CLOCK_SM, std::addressof(smClock));
			if (status != NVML_SUCCESS) {
				throw hwgauge::RecoverableError("NVML get SM clock info failed: {}");
			}
			double gpuFrequency = smClock;

			// Memory Frequency
			unsigned int memClock;
			status = nvmlDeviceGetClockInfo(handle, NVML_CLOCK_MEM, std::addressof(memClock));
			if (status != NVML_SUCCESS) {
				throw hwgauge::RecoverableError("NVML get memory clock info failed");
			}
			double memFrequency = memClock;

			// Power Usage
			unsigned int power;
			status = nvmlDeviceGetPowerUsage(handle, std::addressof(power));
			if (status != NVML_SUCCESS) {
				throw hwgauge::RecoverableError("NVML get power usage failed");
			}
			double power_usage = power / 1e3;  // mW -> W

			metrics.emplace_back(GPUMetrics{
					gpuUtilization,
					memoryUtilization,
					gpuFrequency,
					memFrequency,
					power_usage
				});
		}

		return metrics;
	}
}
#endif