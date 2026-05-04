#pragma once

#ifdef HWGAUGE_USE_NVML

#include <string>

#ifdef HWGAUGE_USE_LOCAL_HTTP
#include <nlohmann/json.hpp>
#endif

namespace hwgauge
{
    struct GPULabel
    {
        std::size_t index;
        std::string name;
    };

    struct GPUMetrics
    {
        double gpuUtilization;
        double memoryUtilization;
        double gpuFrequency;
        double memoryFrequency;
        double powerUsage;

        double temperature;
    };

#ifdef HWGAUGE_USE_LOCAL_HTTP
    inline void to_json(nlohmann::json& j, const GPULabel& l) {
        j = nlohmann::json{{"index", l.index}, {"name", l.name}};
    }

    inline void to_json(nlohmann::json& j, const GPUMetrics& m) {
        j = nlohmann::json{
            {"gpuUtilization", m.gpuUtilization},
            {"memoryUtilization", m.memoryUtilization},
            {"gpuFrequency", m.gpuFrequency},
            {"memoryFrequency", m.memoryFrequency},
            {"powerUsage", m.powerUsage},
            {"temperature", m.temperature}
        };
    }
#endif
}

#endif
