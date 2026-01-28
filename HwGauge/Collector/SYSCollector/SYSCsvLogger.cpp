#ifdef __linux__

#include "SYSCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge {

    std::string SYSCsvLogger::getHeader() const {
        return "MachineName,"
               "MemTotal(GB),MemUsed(GB),MemUtil(%),"
               "DiskRead(MB/s),DiskWrite(MB/s),MaxDiskUtil(%),"
               "NetDown(MB/s),NetUp(MB/s),"
               "SysPower(W)";
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
           << m.systemPowerWatts;

        return ss.str();
    }
}

#endif