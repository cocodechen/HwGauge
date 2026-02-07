#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_INTEL_PCM)

#include "CPUDatabase.hpp"

#include "spdlog/spdlog.h"
#include <iostream>
#include <cstring>

namespace hwgauge
{
    CPUDatabase::CPUDatabase(const DBConfig& config_, const std::string& table_name_prefix)
        : Database<CPULabel, CPUMetrics>(config_)
    {
        // 设置表名
        metric_table_name = table_name_prefix + "_cpu_metric";
        info_table_name = table_name_prefix + "_cpu_info";
        // 创建表
        if (!createMetricTable() || !createInfoTable())throw hwgauge::FatalError("[Database] Create Table Failed");
        // 构建SQL模板
        metric_insert_sql =
            "INSERT INTO " + metric_table_name +
            " (timestamp, cpu_index, cpu_utilization, cpu_frequency, "
            "c0_residency, c6_residency, power_usage, "
            "memory_read_bandwidth, memory_write_bandwidth, memory_power_usage, temperature) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);";

        info_insert_sql =
            "INSERT INTO " + info_table_name +
            " (cpu_index, cpu_name) "
            "VALUES ($1, $2) "
            "ON CONFLICT (cpu_index) DO UPDATE SET "
            "cpu_name = EXCLUDED.cpu_name;";

        spdlog::info("[CPUDatabase] Initialize successfully");
    }

    CPUDatabase::~CPUDatabase(){}

    bool CPUDatabase::createMetricTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + metric_table_name + " ("
            "timestamp TIMESTAMP NOT NULL,"          // 时间戳
            "cpu_index INTEGER NOT NULL,"             // CPU / socket 索引
            "cpu_utilization DOUBLE PRECISION,"       // CPU 利用率(%)
            "cpu_frequency DOUBLE PRECISION,"         // CPU 频率(MHz)
            "c0_residency DOUBLE PRECISION,"          // C0 状态占比(%)
            "c6_residency DOUBLE PRECISION,"          // C6 状态占比(%)
            "power_usage DOUBLE PRECISION,"           // 功耗(W)
            "memory_read_bandwidth DOUBLE PRECISION," // 内存读带宽(MB/s)
            "memory_write_bandwidth DOUBLE PRECISION,"// 内存写带宽(MB/s)
            "memory_power_usage DOUBLE PRECISION,"    // 内存功耗(W)
            "temperature DOUBLE PRECISION"            // 温度(C)
            ");";

        if (!execSQL(sql))
        {
            spdlog::error("[CPUDatabase] Failed to create metric table");
            return false;
        }
        spdlog::info("[CPUDatabase] Table {} created or already exists", metric_table_name);
        return true;
    }

    bool CPUDatabase::createInfoTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + info_table_name + " ("
            "cpu_index INTEGER NOT NULL PRIMARY KEY," // CPU 索引
            "cpu_name TEXT"                           // CPU 名称
            ");";
        if (!execSQL(sql))
        {
            spdlog::error("[CPUDatabase] Failed to create info table");
            return false;
        }
        spdlog::info("[CPUDatabase] Table {} created or already exists", info_table_name);
        return true;
    }

    void CPUDatabase::writeMetric(const std::string& cur_time,
                                const std::vector<CPULabel>& label_list,
                                const std::vector<CPUMetrics>& metric_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[CPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const CPULabel& label = label_list[i];
            const CPUMetrics& metric = metric_list[i];

            std::vector<std::string> buf(11);
            const char* params[11] = {
                to_sql_param_string(cur_time, buf[0]),
                to_sql_param_int(label.index, buf[1]),
                to_sql_param_double(metric.cpuUtilization, buf[2]),
                to_sql_param_double(metric.cpuFrequency, buf[3]),
                to_sql_param_double(metric.c0Residency, buf[4]),
                to_sql_param_double(metric.c6Residency, buf[5]),
                to_sql_param_double(metric.powerUsage, buf[6]),
                to_sql_param_double(metric.memoryReadBandwidth, buf[7]),
                to_sql_param_double(metric.memoryWriteBandwidth, buf[8]),
                to_sql_param_double(metric.memoryPowerUsage, buf[9]),
                to_sql_param_double(metric.temperature, buf[10]),
            };

            if (!execSQL(metric_insert_sql, std::vector<const char*>(params, params + 11)))
            {
                if(useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[CPUDatabase] Successfully inserted {}/{} records into {}", inserted, label_list.size(), metric_table_name);
    }
    
    void CPUDatabase::writeInfo(const std::vector<CPULabel>& label_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[CPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const CPULabel& label = label_list[i];

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
        spdlog::info("[CPUDatabase] Successfully inserted/updated {}/{} static info records into {}", inserted, label_list.size(), info_table_name);
    }
}

#endif
