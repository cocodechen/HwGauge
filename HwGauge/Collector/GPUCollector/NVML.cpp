#ifdef HWGAUGE_USE_NVML

#include "Collector/Exception.hpp"
#include "NVML.hpp"

#include <nvml.h>
#include <array>
#include "spdlog/spdlog.h"

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
			double gpuUtilization,memoryUtilization;
			status = nvmlDeviceGetUtilizationRates(handle, std::addressof(utilization));
			if (status != NVML_SUCCESS)
			{
				gpuUtilization=-1.0;
				memoryUtilization=-1.0;
				spdlog::warn("[NVML] Get utilizations rates failed: {}, {}",nvmlErrorString(status),static_cast<int>(status));
			}
			else
			{
				gpuUtilization = utilization.gpu;
				memoryUtilization = utilization.memory;
			}
			
			// GPU Frequency
			unsigned int smClock;
			double gpuFrequency;
			status = nvmlDeviceGetClockInfo(handle, NVML_CLOCK_SM, std::addressof(smClock));
			if (status != NVML_SUCCESS)
			{
				gpuFrequency=-1.0;
				spdlog::warn("[NVML] Get SM clock info failed: {}, {}",nvmlErrorString(status),static_cast<int>(status));
			}
			else gpuFrequency = smClock;

			// Memory Frequency
			unsigned int memClock;
			double memFrequency;
			status = nvmlDeviceGetClockInfo(handle, NVML_CLOCK_MEM, std::addressof(memClock));
			if (status != NVML_SUCCESS)
			{
				memFrequency=-1.0;
				spdlog::warn("[NVML] Get memory clock info failed: {}, {}",nvmlErrorString(status),static_cast<int>(status));
			}
			else memFrequency = memClock;

			// Power Usage
			unsigned int power;
			double power_usage;
			status = nvmlDeviceGetPowerUsage(handle, std::addressof(power));
			if (status != NVML_SUCCESS)
			{
				power_usage=-1.0;
				spdlog::warn("[NVML] Get power usage failed: {}, {}",nvmlErrorString(status),static_cast<int>(status));
			}
			else power_usage = power / 1e3;  // mW -> W

			// GPU Temperature
			unsigned int temperature; // NVML 返回的温度是无符号整数，单位是摄氏度
			double tempDouble;
			// 第二个参数 NVML_TEMPERATURE_GPU 代表读取核心温度
			status = nvmlDeviceGetTemperature(handle, NVML_TEMPERATURE_GPU, &temperature);
			if (status != NVML_SUCCESS)
			{
				tempDouble = -1.0; 
				spdlog::warn("[NVML] Get temperature failed: {}, {}", nvmlErrorString(status), static_cast<int>(status));
			}
			else tempDouble = static_cast<double>(temperature);

			metrics.emplace_back(GPUMetrics{
					gpuUtilization,
					memoryUtilization,
					gpuFrequency,
					memFrequency,
					power_usage,
					tempDouble
				});
		}

		return metrics;
	}
}
#endif