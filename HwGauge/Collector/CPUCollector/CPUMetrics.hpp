#pragma once

#ifdef HWGAUGE_USE_INTEL_PCM

#include<string>
#include<iostream>

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
	};

    inline void outCPU(const CPULabel& l, const CPUMetrics& m)
    {
        std::cout
            << "CPU{ "
            << "index="     << l.index
            << ", name=" << l.name
            << ", cpuUtilization="   << m.cpuUtilization
            << ", cpuFrequency="   << m.cpuFrequency
            << ", c0Residency=" << m.c0Residency
            << ", c6Residency="  << m.c6Residency
            << ", powerUsage="    << m.powerUsage
            << ", memoryReadBandwidth="  << m.memoryReadBandwidth
            << ", memoryWriteBandwidth="    << m.memoryWriteBandwidth
            << ", memoryPowerUsage=" << m.memoryPowerUsage
            << " }\n";
    }
}

#endif