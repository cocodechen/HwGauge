#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NPU)

#include "NPUDatabase.hpp"

#include "spdlog/spdlog.h"
#include <iostream>
#include <cstring>

namespace hwgauge
{
    NPUDatabase::NPUDatabase(const ConnectionConfig& config_, const std::string& table_name_prefix)
        : Database<NPULabel, NPUMetrics>(config_)
    {
        // 设置表名
        metric_table_name = table_name_prefix + "_npu_metric";
        info_table_name = table_name_prefix + "_npu_info";
        // 创建表
        if (!createMetricTable() || !createInfoTable())throw hwgauge::FatalError("[Database] Create Table Failed");
        // 构建SQL模板
         metric_insert_sql =
            "INSERT INTO " + metric_table_name +
            " (timestamp, card_id, device_id, "
            "freq_aicore, freq_aicpu, "
            "util_aicore, util_aicpu, util_vec, "
            "mem_total_mb, mem_usage_mb, util_mem, util_membw, "
            "chip_power, mcu_power, "
            "health, temperature, voltage) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17);";

        info_insert_sql =
            "INSERT INTO " + info_table_name +
            " (card_id, device_id, chip_type, chip_name) "
            "VALUES ($1, $2, $3, $4) "
            "ON CONFLICT (card_id, device_id) DO UPDATE SET "
            "chip_type = EXCLUDED.chip_type, "
            "chip_name = EXCLUDED.chip_name;";

        spdlog::info("[NPUDatabase] Initialize successfully");
    }

    NPUDatabase::~NPUDatabase(){}

    bool NPUDatabase::createMetricTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + metric_table_name + " ("
            "timestamp TIMESTAMP NOT NULL,"
            "card_id INTEGER NOT NULL,"
            "device_id INTEGER NOT NULL,"
            "freq_aicore INTEGER,"      // AICore频率 (MHz)
            "freq_aicpu INTEGER,"       // AICPU频率 (MHz)
            "util_aicore INTEGER,"      // AICore利用率 (%)
            "util_aicpu INTEGER,"       // AICPU利用率 (%)
            "util_vec INTEGER,"         // Vector Core利用率 (%)
            "mem_total_mb BIGINT,"      // 总显存 (MB)
            "mem_usage_mb BIGINT,"      // 已用显存 (MB)
            "util_mem INTEGER,"         // 显存已用百分比 (%)
            "util_membw INTEGER,"       // 显存带宽利用率 (%)
            "chip_power DOUBLE PRECISION,"  // 芯片功耗 (W)
            "mcu_power DOUBLE PRECISION,"   // MCU功耗 (W)
            "health INTEGER,"           // 健康状态 (0:OK,1:WARN,2:ERROR,3:CRITICAL,0xFFFFFFFF:NOT_EXIST)
            "temperature INTEGER,"      // 温度 (C)
            "voltage DOUBLE PRECISION"  // 电压 (V)
            ");";
        if (!execSQL(sql))
        {
            spdlog::error("[NPUDatabase] Failed to create metric table");
            return false;
        }
        spdlog::info("[NPUDatabase] Table {} created or already exists", metric_table_name);
        return true;
    }

    bool NPUDatabase::createInfoTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + info_table_name + " ("
            "card_id INTEGER NOT NULL,"
            "device_id INTEGER NOT NULL,"
            "chip_type TEXT,"
            "chip_name TEXT,"
            "PRIMARY KEY (card_id, device_id)"
            ");";
        if (!execSQL(sql))
        {
            spdlog::error("[NPUDatabase] Failed to create info table");
            return false;
        }
        spdlog::info("[NPUDatabase] Table {} created or already exists", info_table_name);
        return true;
    }

    void NPUDatabase::writeMetric(const std::string& cur_time,
                                const std::vector<NPULabel>& label_list,
                                const std::vector<NPUMetrics>& metric_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[NPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const NPULabel& label = label_list[i];
            const NPUMetrics& metric = metric_list[i];

            std::vector<std::string> buf(17);
            const char* params[17] = {
                to_sql_param_string(cur_time, buf[0]),
                to_sql_param_int(label.card_id, buf[1]),
                to_sql_param_int(label.device_id, buf[2]),
                // 频率
                to_sql_param_int(metric.freq_aicore, buf[3]),
                to_sql_param_int(metric.freq_aicpu, buf[4]),
                // 算力负载
                to_sql_param_int(metric.util_aicore, buf[5]),
                to_sql_param_int(metric.util_aicpu, buf[6]),
                to_sql_param_int(metric.util_vec, buf[7]),
                // 存储资源
                to_sql_param_long(metric.mem_total_mb, buf[8]),
                to_sql_param_long(metric.mem_usage_mb, buf[9]),
                to_sql_param_int(metric.util_mem, buf[10]),
                to_sql_param_int(metric.util_membw, buf[11]),
                // 功耗
                to_sql_param_double(metric.chip_power, buf[12]),
                to_sql_param_double(metric.mcu_power, buf[13]),
                // 环境
                to_sql_param_int(metric.health, buf[14]),
                to_sql_param_int(metric.temperature, buf[15]),
                to_sql_param_double(metric.voltage, buf[16])
            };

            if (!execSQL(metric_insert_sql, std::vector<const char*>(params, params + 17)))
            {
                if(useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[NPUDatabase] Successfully inserted {}/{} records into {}", inserted, label_list.size(), metric_table_name);
    }
    
    void NPUDatabase::writeInfo(const std::vector<NPULabel>& label_list,
                                bool useTransaction)
    {
        if (!isConnected())throw hwgauge::FatalError("[NPUDatabase] The database hasn't been connected before writing");
        if (useTransaction && !startTransaction())return;
        
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const NPULabel& label = label_list[i];

            std::vector<std::string> buf(4);
            const char* params[4] = {
                to_sql_param_int(label.card_id, buf[0]),
                to_sql_param_int(label.device_id, buf[1]),
                to_sql_param_string(label.chip_type, buf[2]),
                to_sql_param_string(label.chip_name, buf[3])
            };

            if (!execSQL(info_insert_sql, std::vector<const char*>(params, params + 4)))
            {
                if (useTransaction) rollbackTransaction();
                return;
            }
            ++inserted;
        }
        if (useTransaction && !commitTransaction())return;
        spdlog::info("[NPUDatabase] Successfully inserted/updated {}/{} static info records into {}", inserted, label_list.size(), info_table_name);
    }
}

#endif