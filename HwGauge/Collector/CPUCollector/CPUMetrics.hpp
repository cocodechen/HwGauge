#pragma once

#ifdef HWGAUGE_USE_INTEL_PCM

#include <string>

#ifdef HWGAUGE_USE_LOCAL_HTTP
#include <nlohmann/json.hpp>
#endif

namespace hwgauge
{
    struct CPULabel
    {
		std::size_t index;
		std::string name;
	};

	struct CPUMetrics
    {
		double cpuUtilization;         // CPU utilization percentage
		double cpuFrequency;           // CPU frequency in MHz
		double c0Residency;            // C0 state residency percentage
		double c6Residency;            // C6 state residency percentage
		double powerUsage;             // CPU power usage in watts
		double memoryReadBandwidth;    // Memory read bandwidth in MB/s
		double memoryWriteBandwidth;   // Memory write bandwidth in MB/s
		double memoryPowerUsage;       // Memory power usage in watts

        double temperature;            // temperature
	};

#ifdef HWGAUGE_USE_LOCAL_HTTP
    // 定义序列化规则（必须与结构体在同一命名空间）
    inline void to_json(nlohmann::json& j, const CPULabel& l) {
        j = nlohmann::json{{"index", l.index}, {"name", l.name}};
    }

    inline void to_json(nlohmann::json& j, const CPUMetrics& m) {
        j = nlohmann::json{
            {"cpuUtilization", m.cpuUtilization},
            {"cpuFrequency", m.cpuFrequency},
            {"c0Residency", m.c0Residency},
            {"c6Residency", m.c6Residency},
            {"powerUsage", m.powerUsage},
            {"memoryReadBandwidth", m.memoryReadBandwidth},
            {"memoryWriteBandwidth", m.memoryWriteBandwidth},
            {"memoryPowerUsage", m.memoryPowerUsage},
            {"temperature", m.temperature}
        };
    }
#endif


}

#endif