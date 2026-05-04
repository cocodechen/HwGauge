#pragma once

#ifdef HWGAUGE_USE_NVML

#include "Collector/Base/DeviceCollector.hpp"
#include "NVML.hpp"
#include "GPUDatabase.hpp"
#include "GPUCsvLogger.hpp"
#include "GPUPrometheus.hpp"

#include <iostream>

namespace hwgauge
{
#ifdef HWGAUGE_USE_POSTGRESQL
    using GPUDatabaseType = GPUDatabase;
#else
    using GPUDatabaseType = NullType;
#endif

#ifdef HWGAUGE_USE_PROMETHEUS
    using GPUPrometheusType = GPUPrometheus;
#else
    using GPUPrometheusType = NullType;
#endif

#ifdef HWGAUGE_USE_LOCAL_HTTP
    using GPUHttpApiType = HttpApi<GPULabel, GPUMetrics>;
#else
    using GPUHttpApiType = NullType;
#endif

    // 定义别名
    using GPUCollector = DeviceCollector<
        GPULabel, GPUMetrics, NVML, GPUDatabaseType, GPUCsvLogger, GPUPrometheusType, GPUHttpApiType
    >;
    
    // 定义特定的打印函数
    template<>
    inline void printMetric(const GPULabel& l, const GPUMetrics& m)
    {
        std::cout
            << "GPU{ "
            << "index="    << l.index
            << ", name="   << l.name
            << ", util="   << m.gpuUtilization
            << ", memUtil="<< m.memoryUtilization
            << ", gpuFreq="<< m.gpuFrequency
            << ", memFreq="<< m.memoryFrequency
            << ", power="  << m.powerUsage
            << ", temp="   << m.temperature<<"C"
            << " }\n";
    }

    // 定义全局信息设置函数
    template<>
    inline void setContextInfo(std::vector<GPULabel>& l, std::vector<GPUMetrics>& m)
    {
        for(size_t i=0; i<l.size(); i++) 
        {
            sharedPower.gpu_power = sharedPower.gpu_power.value_or(0.0) + m[i].powerUsage;
        }
    }

}

#endif