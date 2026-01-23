#pragma once
#if defined(HWGAUGE_USE_POSTGRESQL) && defined(__linux__)

#include "SYSMetrics.hpp"
#include "Collector/Config.hpp"
#include "Collector/Database.hpp"

namespace hwgauge
{
    /* SYS数据库操作类，继承自Database */
    class SYSDatabase : public Database<SYSLabel, SYSMetrics>
    {
    public:
        /* 构造函数 */
        explicit SYSDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix);
        
        /* 析构函数 */
        ~SYSDatabase();
        
        /* 写入SYS监控数据 */
        void writeMetric(const std::string& cur_time,
                        const std::vector<SYSLabel>& label_list, 
                        const std::vector<SYSMetrics>& metric_list,
                        bool useTransaction = true) override;
        
        /* 写入SYS静态数据 */
        void writeInfo(const std::vector<SYSLabel>& label_list,
                      bool useTransaction = true) override;
        
    private:
        
        /* 创建指标数据表 */
        bool createMetricTable() override;
        /* 创建静态数据表 */
        bool createInfoTable() override;
    };
}

#endif