#pragma once

#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NVML)

#include "GPUMetrics.hpp"
#include "Collector/Config.hpp"
#include "Collector/Database.hpp"

namespace hwgauge
{
    /* GPU数据库操作类，继承自Database */
    class GPUDatabase : public Database<GPULabel, GPUMetrics>
    {
    public:
        /* 构造函数 */
        explicit GPUDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix);
        
        /* 析构函数 */
        ~GPUDatabase();
        
        /* 写入GPU监控数据 */
        void writeMetric(const std::string& cur_time,
                        const std::vector<GPULabel>& label_list, 
                        const std::vector<GPUMetrics>& metric_list,
                        bool useTransaction = true) override;
        
        /* 写入GPU静态数据 */
        void writeInfo(const std::vector<GPULabel>& label_list,
                      bool useTransaction = true) override;
        
    private:
        
        /* 创建指标数据表 */
        bool createMetricTable() override;
        /* 创建静态数据表 */
        bool createInfoTable() override;
    };
}

#endif