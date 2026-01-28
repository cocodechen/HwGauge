#pragma once
#ifdef HWGAUGE_USE_INTEL_PCM

#include "Collector/CsvLogger.hpp"
#include "CPUMetrics.hpp"

namespace hwgauge
{
    class CPUCsvLogger : public CsvLogger<CPULabel, CPUMetrics>
    {
    public:
        using CsvLogger<CPULabel, CPUMetrics>::CsvLogger;
    protected:
        std::string getHeader() const override;
        std::string formatRow(const CPULabel& l, const CPUMetrics& m) const override;
    };
}

#endif