#ifdef __linux__

#include "SYSCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge {

    SYSCsvLogger::SYSCsvLogger(const std::string& filepath) : CsvLogger(filepath) 
    {
        auto pos = m_filepath.rfind(".csv");
        if (pos != std::string::npos) {
            m_filepath.insert(pos, "_sys");
        }

        m_ofs.open(m_filepath, std::ios::out | std::ios::app);
        
        if (!m_ofs.is_open()) {
            spdlog::error("[SYSCsvLogger] Failed to open file: {}", m_filepath);
            throw FatalError("SYSCsvLogger open failed: " + m_filepath);
        }
        spdlog::info("[SYSCsvLogger] Initialized logger for: {}", m_filepath);
    }

    std::string SYSCsvLogger::getHeader() const {
        return "MachineName,"
               "MemTotal(GB),MemUsed(GB),MemUtil(%),"
               "DiskRead(MB/s),DiskWrite(MB/s),MaxDiskUtil(%),"
               "NetDown(MB/s),NetUp(MB/s),"
               "SysPower(W),TotalPower(W)";
    }

    std::string SYSCsvLogger::formatRow(const SYSLabel& l, const SYSMetrics& m) const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);

        ss << "\"" << l.name << "\"," // 机器名可能有空格
           << m.memTotalGB << ","
           << m.memUsedGB << ","
           << m.memUtilizationPercent << ","
           << m.diskReadMBps << ","
           << m.diskWriteMBps << ","
           << m.maxDiskUtilPercent << ","
           << m.netDownloadMBps << ","
           << m.netUploadMBps << ","
           << m.systemPowerWatts << ","
           << m.totalPowerWatts;

        return ss.str();
    }
}

#endif