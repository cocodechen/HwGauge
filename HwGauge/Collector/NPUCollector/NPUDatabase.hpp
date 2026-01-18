#pragma once
#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NPU)

#include "NPUMetrics.hpp"
#include "Collector/Config.hpp"
#include "Collector/Database.hpp"

namespace hwgauge
{
    /* NPU数据库操作类，继承自Database */
    class NPUDatabase : public Database<NPULabel, NPUMetrics>
    {
    public:
        /* 构造函数 */
        explicit NPUDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix);
        
        /* 析构函数 */
        ~NPUDatabase();
        
        /* 写入NPU监控数据 */
        void writeMetric(const std::vector<NPULabel>& label_list, 
                        const std::vector<NPUMetrics>& metric_list,
                        bool useTransaction = true) override;
        
        /* 写入NPU静态数据 */
        void writeInfo(const std::vector<NPULabel>& label_list,
                      bool useTransaction = true) override;
        
    private:
        
        /* 创建指标数据表 */
        bool createMetricTable() override;
        /* 创建静态数据表 */
        bool createInfoTable() override;
    };
}

#endif