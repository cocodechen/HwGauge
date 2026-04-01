#pragma once
#ifdef __linux__

#include "Collector/Base/CsvLogger.hpp"
#include "SYSMetrics.hpp"

namespace hwgauge {
    class SYSCsvLogger : public CsvLogger<SYSLabel, SYSMetrics> {
    public:
        explicit SYSCsvLogger(const std::string& filepath);
    protected:
        std::string getHeader() const override;
        std::string formatRow(const SYSLabel& l, const SYSMetrics& m) const override;
    };
}
#endif