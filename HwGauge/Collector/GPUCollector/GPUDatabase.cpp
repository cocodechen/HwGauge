#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NVML)

#include "GPUDatabase.hpp"

#include "spdlog/spdlog.h"
#include <iostream>
#include <cstring>

namespace hwgauge
{
    GPUDatabase::GPUDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix)
        : Database<GPULabel, GPUMetrics>(config_)
    {
        // 设置表名
        metric_table_name = table_name_prefix + "_gpu_metric";
        info_table_name = table_name_prefix + "_gpu_info";
        // 创建表
        if (!createMetricTable() || !createInfoTable())throw hwgauge::FatalError("[Database] Create Table Failed");
        // 构建SQL模板
        metric_insert_sql =
            "INSERT INTO " + metric_table_name +
            " (timestamp, gpu_index, gpu_utilization, memory_utilization, "
            "gpu_frequency, memory_frequency, power_usage, temperature) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8);";

        info_insert_sql =
            "INSERT INTO " + info_table_name +
            " (gpu_index, gpu_name) "
            "VALUES ($1, $2) "
            "ON CONFLICT (gpu_index) DO UPDATE SET "
            "gpu_name = EXCLUDED.gpu_name;";

        spdlog::info("[GPUDatabase] Initialize successfully");
    }

    GPUDatabase::~GPUDatabase(){}

    bool GPUDatabase::createMetricTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + metric_table_name + " ("
            "timestamp TIMESTAMP NOT NULL,"             // 时间戳
            "gpu_index INTEGER NOT NULL,"               // GPU索引
            "gpu_utilization DOUBLE PRECISION,"         // GPU利用率(%)
            "memory_utilization DOUBLE PRECISION,"      // 显存利用率(%)
            "gpu_frequency DOUBLE PRECISION,"           // GPU频率(MHz)
            "memory_frequency DOUBLE PRECISION,"        // 显存频率(MHz)
            "power_usage DOUBLE PRECISION,"             // 功耗(W)
            "temperature DOUBLE PRECISION"              //温度(C)
            ");";
        if (!execSQL(sql))
        {
            spdlog::error("[GPUDatabase] Failed to create metric table");
            return false;
        }
        spdlog::info("[GPUDatabase] Table {} created or already exists", metric_table_name);
        return true;
    }

    bool GPUDatabase::createInfoTable()
    {
        const std::string sql =
        "CREATE TABLE IF NOT EXISTS " + info_table_name + " ("
        "gpu_index INTEGER NOT NULL PRIMARY KEY,"  // GPU索引，主键
        "gpu_name TEXT"                            // GPU名称
        ");";
        if (!execSQL(sql))
        {
            spdlog::error("[GPUDatabase] Failed to create info table");
            return false;
        }
        spdlog::info("[GPUDatabase] Table {} created or already exists", info_table_name);
        return true;
    }

    void GPUDatabase::writeMetric(const std::string& cur_time,
                                const std::vector<GPULabel>& label_list,
                                const std::vector<GPUMetrics>& metric_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[GPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const GPULabel& label = label_list[i];
            const GPUMetrics& metric = metric_list[i];

            std::vector<std::string> buf(8);
            const char* params[8] = {
                to_sql_param_string(cur_time, buf[0]),
                to_sql_param_int(label.index, buf[1]),
                to_sql_param_double(metric.gpuUtilization, buf[2]),
                to_sql_param_double(metric.memoryUtilization, buf[3]),
                to_sql_param_double(metric.gpuFrequency, buf[4]),
                to_sql_param_double(metric.memoryFrequency, buf[5]),
                to_sql_param_double(metric.powerUsage, buf[6]),
                to_sql_param_double(metric.temperature,buf[7]),
            };

            if (!execSQL(metric_insert_sql, std::vector<const char*>(params, params +8)))
            {
                if(useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[GPUDatabase] Successfully inserted {}/{} records into {}", inserted, label_list.size(), metric_table_name);
    }
    
    void GPUDatabase::writeInfo(const std::vector<GPULabel>& label_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[GPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const GPULabel& label = label_list[i];

            std::vector<std::string> buf(2);
            const char* params[2] = {
                to_sql_param_int(label.index, buf[0]),
                to_sql_param_string(label.name, buf[1]),
            };

            if (!execSQL(info_insert_sql, std::vector<const char*>(params, params + 2)))
            {
                if (useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[GPUDatabase] Successfully inserted/updated {}/{} static info records into {}", inserted, label_list.size(), info_table_name);
    }
}

#endif
