#pragma once

#include "prometheus/gauge.h"
#include "prometheus/family.h"
#include "Collector/Collector.hpp"
#include <string>
#include <utility>
#include <vector>
#include <ranges>

namespace hwgauge {
	struct CPULabel {
		std::size_t index;
		std::string name;
	};

	struct CPUMetrics {
		double cpuFrequency;           // CPU frequency in MHz
		double c0Residency;            // C0 state residency percentage
		double c6Residency;            // C6 state residency percentage
		double powerUsage;             // CPU power usage in watts
		double memoryReadBandwidth;    // Memory read bandwidth in MB/s
		double memoryWriteBandwidth;   // Memory write bandwidth in MB/s
		double memoryPowerUsage;       // Memory power usage in watts
	};

	template <typename T>
	class CPUCollector : public Collector {
	public:
		explicit CPUCollector(std::shared_ptr<Registry> registry, T impl) :
			Collector(registry), impl(std::move(impl)),
			cpuFrequencyFamily(prometheus::BuildGauge().Name("cpu_frequency_mhz").Help("CPU frequency in MHz").Register(*registry)),
			c0ResidencyFamily(prometheus::BuildGauge().Name("cpu_c0_residency_percent").Help("CPU C0 state residency percentage").Register(*registry)),
			c6ResidencyFamily(prometheus::BuildGauge().Name("cpu_c6_residency_percent").Help("CPU C6 state residency percentage").Register(*registry)),
			powerUsageFamily(prometheus::BuildGauge().Name("cpu_power_usage_watts").Help("CPU power usage in watts").Register(*registry)),
			memoryReadBandwidthFamily(prometheus::BuildGauge().Name("memory_read_bandwidth_mbps").Help("Memory read bandwidth in MB/s").Register(*registry)),
			memoryWriteBandwidthFamily(prometheus::BuildGauge().Name("memory_write_bandwidth_mbps").Help("Memory write bandwidth in MB/s").Register(*registry)),
			memoryPowerUsageFamily(prometheus::BuildGauge().Name("memory_power_usage_watts").Help("Memory power usage in watts").Register(*registry))
		{
			for (auto& label : labels()) {
				cpuFrequencyGauges.push_back(std::addressof(cpuFrequencyFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				c0ResidencyGauges.push_back(std::addressof(c0ResidencyFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				c6ResidencyGauges.push_back(std::addressof(c6ResidencyFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				powerUsageGauges.push_back(std::addressof(powerUsageFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				memoryReadBandwidthGauges.push_back(std::addressof(memoryReadBandwidthFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				memoryWriteBandwidthGauges.push_back(std::addressof(memoryWriteBandwidthFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
				memoryPowerUsageGauges.push_back(std::addressof(memoryPowerUsageFamily.Add({ { "index", std::to_string(label.index) }, { "name", label.name } })));
			}
		}
		virtual ~CPUCollector() = default;

		void collect() override {
			for (auto [index, metrics] : std::views::enumerate(sample())) {
				cpuFrequencyGauges[index]->Set(metrics.cpuFrequency);
				c0ResidencyGauges[index]->Set(metrics.c0Residency);
				c6ResidencyGauges[index]->Set(metrics.c6Residency);
				powerUsageGauges[index]->Set(metrics.powerUsage);
				memoryReadBandwidthGauges[index]->Set(metrics.memoryReadBandwidth);
				memoryWriteBandwidthGauges[index]->Set(metrics.memoryWriteBandwidth);
				memoryPowerUsageGauges[index]->Set(metrics.memoryPowerUsage);
			}
		}

		std::string name() override { return impl.name(); }
		std::vector<CPULabel> labels() { return impl.labels(); }
		std::vector<CPUMetrics> sample() { return impl.sample(); }

	private:
		T impl;

		prometheus::Family<prometheus::Gauge>& cpuFrequencyFamily;
		std::vector<prometheus::Gauge*> cpuFrequencyGauges;

		prometheus::Family<prometheus::Gauge>& c0ResidencyFamily;
		std::vector<prometheus::Gauge*> c0ResidencyGauges;

		prometheus::Family<prometheus::Gauge>& c6ResidencyFamily;
		std::vector<prometheus::Gauge*> c6ResidencyGauges;

		prometheus::Family<prometheus::Gauge>& powerUsageFamily;
		std::vector<prometheus::Gauge*> powerUsageGauges;

		prometheus::Family<prometheus::Gauge>& memoryReadBandwidthFamily;
		std::vector<prometheus::Gauge*> memoryReadBandwidthGauges;

		prometheus::Family<prometheus::Gauge>& memoryWriteBandwidthFamily;
		std::vector<prometheus::Gauge*> memoryWriteBandwidthGauges;

		prometheus::Family<prometheus::Gauge>& memoryPowerUsageFamily;
		std::vector<prometheus::Gauge*> memoryPowerUsageGauges;
	};
}