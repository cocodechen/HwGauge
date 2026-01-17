#pragma once

#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_INTEL_PCM)

#include "CPUMetrics.hpp"
#include "Collector/DBConfig.hpp"
#include "Collector/Database.hpp"

namespace hwgauge
{
    /* CPU数据库操作类，继承自Database */
    class CPUDatabase : public Database<CPULabel, CPUMetrics>
    {
    public:
        /* 构造函数 */
        explicit CPUDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix);
        
        /* 析构函数 */
        ~CPUDatabase();
        
        /* 写入CPU监控数据 */
        void writeMetric(const std::vector<CPULabel>& label_list, 
                        const std::vector<CPUMetrics>& metric_list,
                        bool useTransaction = true) override;
        
        /* 写入CPU静态数据 */
        void writeInfo(const std::vector<CPULabel>& label_list,
                      bool useTransaction = true) override;
        
    private:
        
        /* 创建指标数据表 */
        bool createMetricTable() override;
        /* 创建静态数据表 */
        bool createInfoTable() override;
    };
}

#endif