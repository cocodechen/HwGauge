#pragma once

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include <string>
#include <utility>
#include <vector>
#include <ranges>

namespace hwgauge {
	struct GPULabel {
		std::size_t index;
		std::string name;
	};

	struct GPUMetrics {
		double gpuUtilization;
		double memoryUtilization;
		double gpuFrequency;
		double memoryFrequency;
		double powerUsage;
	};

	template <typename T>
	class GPUCollector : public Collector {
	public:
		explicit GPUCollector(std::shared_ptr<Registry> registry, T impl) :
			Collector(registry), impl(std::move(impl)),
			gpuUtilizationFamily(prometheus::BuildGauge().Name("gpu_utilization_percent").Help("GPU utilization percentage").Register(*registry)),
			memoryUtilizationFamily(prometheus::BuildGauge().Name("gpu_memory_utilization_percent").Help("GPU memory utilization percentage").Register(*registry)),
			gpuFrequencyFamily(prometheus::BuildGauge().Name("gpu_frequency_mhz").Help("GPU frequency in MHz").Register(*registry)),
			memoryFrequencyFamily(prometheus::BuildGauge().Name("gpu_memory_frequency_mhz").Help("GPU memory frequency in MHz").Register(*registry)),
			powerUsageFamily(prometheus::BuildGauge().Name("gpu_power_usage_watts").Help("GPU power usage in watts").Register(*registry))
		{
			for (auto& label : labels()) {
				gpuUtilizationGauges.push_back(std::addressof(gpuUtilizationFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				memoryUtilizationGauges.push_back(std::addressof(memoryUtilizationFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				gpuFrequencyGauges.push_back(std::addressof(gpuFrequencyFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				memoryFrequencyGauges.push_back(std::addressof(memoryFrequencyFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				powerUsageGauges.push_back(std::addressof(powerUsageFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
			}
		}
		virtual ~GPUCollector() = default;

		void collect() override {
			for (auto [index, metrics] : std::views::enumerate(sample())) {
				gpuUtilizationGauges[index]->Set(metrics.gpuUtilization);
				memoryUtilizationGauges[index]->Set(metrics.memoryUtilization);
				gpuFrequencyGauges[index]->Set(metrics.gpuFrequency);
				memoryFrequencyGauges[index]->Set(metrics.memoryFrequency);
				powerUsageGauges[index]->Set(metrics.powerUsage);
			}
		}

		std::string name() override { return impl.name(); }
		std::vector<GPULabel> labels() { return impl.labels(); }
		std::vector<GPUMetrics> sample() { return impl.sample(); }

	private:
		T impl;

		prometheus::Family<prometheus::Gauge>& gpuUtilizationFamily;
		std::vector<prometheus::Gauge*> gpuUtilizationGauges;

		prometheus::Family<prometheus::Gauge>& memoryUtilizationFamily;
		std::vector<prometheus::Gauge*> memoryUtilizationGauges;

		prometheus::Family<prometheus::Gauge>& gpuFrequencyFamily;
		std::vector<prometheus::Gauge*> gpuFrequencyGauges;

		prometheus::Family<prometheus::Gauge>& memoryFrequencyFamily;
		std::vector<prometheus::Gauge*> memoryFrequencyGauges;

		prometheus::Family<prometheus::Gauge>& powerUsageFamily;
		std::vector<prometheus::Gauge*> powerUsageGauges;
	};
}