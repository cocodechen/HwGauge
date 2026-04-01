#ifdef HWGAUGE_USE_INTEL_PCM

#include "CPUCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge
{
    CPUCsvLogger::CPUCsvLogger(const std::string& filepath) : CsvLogger(filepath) 
    {
        auto pos = m_filepath.rfind(".csv");
        if (pos != std::string::npos) {
            m_filepath.insert(pos, "_cpu");
        }

        m_ofs.open(m_filepath, std::ios::out | std::ios::app);
        
        if (!m_ofs.is_open()) {
            spdlog::error("[CPUCsvLogger] Failed to open file: {}", m_filepath);
            throw FatalError("CPUCsvLogger open failed: " + m_filepath);
        }
        spdlog::info("[CPUCsvLogger] Initialized logger for: {}", m_filepath);
    }

    std::string CPUCsvLogger::getHeader() const {
        return "Index,Name,Util(%),Freq(MHz),Temp(C),Power(W),"
               "C0(%),C6(%),MemRead(MB/s),MemWrite(MB/s),MemPower(W)";
    }

    std::string CPUCsvLogger::formatRow(const CPULabel& l, const CPUMetrics& m) const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2); // 统一设置浮点精度

        ss << l.index << ","
           << "\"" << l.name << "\","
           << m.cpuUtilization << ","
           << m.cpuFrequency << ","
           << m.temperature << ","
           << m.powerUsage << ","
           << m.c0Residency << ","
           << m.c6Residency << ","
           << m.memoryReadBandwidth << ","
           << m.memoryWriteBandwidth << ","
           << m.memoryPowerUsage;
        
        return ss.str();
    }
}
#endif