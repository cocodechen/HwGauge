#pragma once

#ifdef HWGAUGE_USE_NVML

#include "Collector/Base/CsvLogger.hpp"
#include "GPUMetrics.hpp"

namespace hwgauge
{
    // 继承自模板基类
    class GPUCsvLogger : public CsvLogger<GPULabel, GPUMetrics>
    {
    public:
        // 使用基类的构造函数
        using CsvLogger<GPULabel, GPUMetrics>::CsvLogger;
        ~GPUCsvLogger() override = default;

    protected:
        // 声明虚函数，在 .cpp 中实现
        std::string getHeader() const override;
        std::string formatRow(const GPULabel& l, const GPUMetrics& m) const override;
    };
}
#endif