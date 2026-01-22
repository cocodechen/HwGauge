#ifdef HWGAUGE_USE_POSTGRESQL

#include "SYSDatabase.hpp"

#include "spdlog/spdlog.h"
#include <iostream>
#include <cstring>

namespace hwgauge
{
    SYSDatabase::SYSDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix)
        : Database<SYSLabel, SYSMetrics>(config_)
    {
        // 设置表名
        metric_table_name = table_name_prefix + "_sys_metric";
        info_table_name = table_name_prefix + "_sys_info";
        // 创建表
        if (!createMetricTable() || !createInfoTable())throw hwgauge::FatalError("[Database] Create Table Failed");
        // 构建SQL模板
        metric_insert_sql =
            "INSERT INTO "+ metric_table_name +
            "(timestamp, mem_total_gb, mem_used_gb, mem_util_percent, "
            "disk_read_mbps, disk_write_mbps, max_disk_util_percent, "
            "net_download_mbps, net_upload_mbps, system_power_watts) " 
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10);";

        spdlog::info("[SYSDatabase] Initialize successfully");
    }

    SYSDatabase::~SYSDatabase(){}

    bool SYSDatabase::createMetricTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + metric_table_name + " ("
            "timestamp TIMESTAMP NOT NULL,"
            "mem_total_gb DOUBLE PRECISION,"
            "mem_used_gb DOUBLE PRECISION,"
            "mem_util_percent DOUBLE PRECISION,"
            "disk_read_mbps DOUBLE PRECISION,"
            "disk_write_mbps DOUBLE PRECISION,"
            "max_disk_util_percent DOUBLE PRECISION,"
            "net_download_mbps DOUBLE PRECISION,"
            "net_upload_mbps DOUBLE PRECISION,"
            "system_power_watts DOUBLE PRECISION,"
            "PRIMARY KEY (timestamp)"
            ");";
        if (!execSQL(sql))
        {
            spdlog::error("[SYSDatabase] Failed to create metric table");
            return false;
        }
        spdlog::info("[SYSDatabase] Table {} created or already exists", metric_table_name);
        return true;
    }

    bool SYSDatabase::createInfoTable()
    {
        spdlog::info("[SYSDatabase] Don't need info table: {}", info_table_name);
        return true;
    }

    void SYSDatabase::writeMetric(const std::string& cur_time,
                                const std::vector<SYSLabel>& label_list,
                                const std::vector<SYSMetrics>& metric_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[SYSDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const SYSLabel& label = label_list[i];
            const SYSMetrics& metric = metric_list[i];

            std::vector<std::string> buf(10);
            const char* params[10] = {
                to_sql_param_string(cur_time, buf[0]),
                to_sql_param_double(metric.memTotalGB, buf[1]),
                to_sql_param_double(metric.memUsedGB, buf[2]),
                to_sql_param_double(metric.memUtilizationPercent, buf[3]),
                to_sql_param_double(metric.diskReadMBps, buf[4]),
                to_sql_param_double(metric.diskWriteMBps, buf[5]),
                to_sql_param_double(metric.maxDiskUtilPercent, buf[6]),
                to_sql_param_double(metric.netDownloadMBps, buf[7]),
                to_sql_param_double(metric.netUploadMBps, buf[8]),
                to_sql_param_double(metric.systemPowerWatts, buf[9]),
            };

            if (!execSQL(metric_insert_sql, std::vector<const char*>(params, params + 10)))
            {
                if(useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[SYSDatabase] Successfully inserted {}/{} records into {}", inserted, label_list.size(), metric_table_name);
    }
    
    void SYSDatabase::writeInfo(const std::vector<SYSLabel>& label_list,
                                bool useTransaction)
    {
        spdlog::info("[SYSDatabase] Don't need to insert info table: {}",info_table_name);
    }
}

#endif