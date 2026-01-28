#pragma once
#ifdef HWGAUGE_USE_NPU

#include "Collector/Base/CsvLogger.hpp"
#include "NPUMetrics.hpp"

namespace hwgauge {
    class NPUCsvLogger : public CsvLogger<NPULabel, NPUMetrics> {
    public:
        using CsvLogger<NPULabel, NPUMetrics>::CsvLogger;
    protected:
        std::string getHeader() const override;
        std::string formatRow(const NPULabel& l, const NPUMetrics& m) const override;
    };
}
#endif