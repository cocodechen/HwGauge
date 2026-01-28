#pragma once
#ifdef __linux__

#include "Collector/CsvLogger.hpp"
#include "SYSMetrics.hpp"

namespace hwgauge {
    class SYSCsvLogger : public CsvLogger<SYSLabel, SYSMetrics> {
    public:
        using CsvLogger<SYSLabel, SYSMetrics>::CsvLogger;
    protected:
        std::string getHeader() const override;
        std::string formatRow(const SYSLabel& l, const SYSMetrics& m) const override;
    };
}
#endif