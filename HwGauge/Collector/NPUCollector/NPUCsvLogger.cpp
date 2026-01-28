#ifdef HWGAUGE_USE_NPU

#include "NPUCsvLogger.hpp"
#include <sstream>
#include <iomanip>

namespace hwgauge {

    std::string NPUCsvLogger::getHeader() const {
        return "CardID,DevID,Type,Name,"
               "FreqAICore(MHz),FreqAICPU(MHz),FreqCtrl(MHz),"
               "UtilAICore(%),UtilAICPU(%),UtilCtrl(%),UtilVec(%),"
               "MemTotal(MB),MemUsed(MB),UtilMem(%),UtilMemBW(%),FreqMem(MHz),"
               "Power(W),Temp(C),Volt(V),Health";
    }

    std::string NPUCsvLogger::formatRow(const NPULabel& l, const NPUMetrics& m) const {
        std::stringstream ss;
        
        ss << l.card_id << ","
           << l.device_id << ","
           << "\"" << l.chip_type << "\","
           << "\"" << l.chip_name << "\","
           
           // 频率
           << m.freq_aicore << ","
           << m.freq_aicpu << ","
           << m.freq_ctrlcpu << ","
           
           // 负载利用率
           << m.util_aicore << ","
           << m.util_aicpu << ","
           << m.util_ctrlcpu << ","
           << m.util_vec << ","
           
           // 存储
           << m.mem_total_mb << ","
           << m.mem_usage_mb << ","
           << m.util_mem << ","
           << m.util_membw << ","
           << m.freq_mem << ","
           
           // 功耗环境 (浮点数)
           << std::fixed << std::setprecision(2) << m.chip_power << ","
           << m.temperature << ","  // int
           << m.voltage << ","      // double
           << m.health;             // unsigned int (hex might be better? keeping decimal for csv)

        return ss.str();
    }
}
#endif