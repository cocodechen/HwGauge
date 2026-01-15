#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NPU)

#include "NPUDatabase.hpp"

#include "spdlog/spdlog.h"
#include <iostream>
#include <cstring>

namespace hwgauge
{
    NPUDatabase::NPUDatabase(const ConnectionConfig& config_, const std::string table_name_)
        : conn(nullptr), config(config_)
    {
        // 建立连接
        if (!connect())exit(EXIT_FAILURE);

        // 创建指标数据表
        metric_table_name=table_name_+"_metric";
        if (!createMetricTable())exit(EXIT_FAILURE);

        // 创建静态数据表
        info_table_name=table_name_+"_info";
        if (!createInfoTable())exit(EXIT_FAILURE);

        // 构建类成员SQL模板
        metric_insert_sql =
            "INSERT INTO " + metric_table_name +
            " (timestamp, card_id, device_id, util_aicore, util_aicpu, util_mem, util_membw, util_vec,"
            "freq_aicore, freq_aicpu, freq_mem, power, health, temperature, voltage) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15);";

        info_insert_sql =
            "INSERT INTO " + info_table_name +
            " (card_id, device_id, chip_type, chip_name) "
            "VALUES ($1, $2, $3, $4) "
            "ON CONFLICT (card_id, device_id) DO UPDATE SET "
            "chip_type = EXCLUDED.chip_type, "
            "chip_name = EXCLUDED.chip_name;";

        spdlog::info("[NPUDatabase] Initialize successfully");
    }

    NPUDatabase::~NPUDatabase()
    {
        disconnect();
    }

    bool NPUDatabase::isConnected() const
    {
        if(conn != nullptr && PQstatus(conn) == CONNECTION_OK)return true;
        else
        {
            spdlog::warn("[NPUDatabase] Database has not connected");
            return false;
        }
    }

    void NPUDatabase::writeMetric(const std::vector<NPULabel>& label_list,
                    const std::vector<NPUMetrics>& metric_list,
                    bool useTransaction)
    {
        if (!isConnected())exit(EXIT_FAILURE);
        // 开启事务
        if (useTransaction)
        {
            if(!startTransaction())return;
        } 

        auto timestamp = getNowTime();
        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const NPULabel& label = label_list[i];
            const NPUMetrics& metric = metric_list[i];

            std::vector<std::string>buf(15);
            const char* params[15] = {
                to_sql_param_string(timestamp,buf[0]),
                to_sql_param_int(label.card_id, buf[1]),
                to_sql_param_int(label.device_id, buf[2]),
                to_sql_param_int(metric.util_aicore, buf[3]),
                to_sql_param_int(metric.util_aicpu, buf[4]),
                to_sql_param_int(metric.util_mem, buf[5]),
                to_sql_param_int(metric.util_membw, buf[6]),
                to_sql_param_int(metric.util_vec, buf[7]),
                to_sql_param_int(metric.freq_aicore, buf[8]),
                to_sql_param_int(metric.freq_aicpu, buf[9]),
                to_sql_param_int(metric.freq_mem, buf[10]),
                to_sql_param_double(metric.power, buf[11]),
                to_sql_param_int(metric.health, buf[12]),
                to_sql_param_int(metric.temperature, buf[13]),
                to_sql_param_double(metric.voltage, buf[14])
            };

            if (!execSQL(metric_insert_sql, std::vector<const char*>(params, params + 15)))
            {
                if (useTransaction)
                {
                    PQexec(conn, "ROLLBACK");
                    spdlog::warn("[NPUDatabase] Transaction rolled back due to previous error");
                }
                return;
            }
            ++inserted;
        }
        // 提交事务
        if (useTransaction)
        {
            if(!commitTransaction())return;
        } 
        spdlog::info("[NPUDatabase] Successfully inserted {}/{} records into {}", inserted, label_list.size(), metric_table_name);
    }
    
    void NPUDatabase::writeInfo(const std::vector<NPULabel>& label_list,
                const std::vector<NPUInfo>& info_list,
                bool useTransaction)
    {
        if (!isConnected()) exit(EXIT_FAILURE);
        // 开启事务
        if (useTransaction)
        {
            if(!startTransaction())return;
        } 

        int inserted = 0;
        for (size_t i = 0; i < label_list.size(); ++i)
        {
            const NPULabel& label = label_list[i];
            const NPUInfo& info   = info_list[i];

            std::vector<std::string>buf(4);
            const char* params[4] = {
                to_sql_param_int(label.card_id, buf[0]),
                to_sql_param_int(label.device_id, buf[1]),
                to_sql_param_string(info.chip_type, buf[2]),
                to_sql_param_string(info.chip_name, buf[3])
            };

            if (!execSQL(info_insert_sql, std::vector<const char*>(params, params + 4)))
            {
                if (useTransaction)
                {
                    PQexec(conn, "ROLLBACK");
                    spdlog::warn("[NPUDatabase] Transaction rolled back due to previous error");
                }
                return;
            }
            ++inserted;
        }
        // 提交事务
        if (useTransaction)
        {
            if(!commitTransaction())return;
        } 

        spdlog::info("[NPUDatabase] Successfully inserted/updated {}/{} static info records into {}", inserted, label_list.size(), info_table_name);
    }
    
    bool NPUDatabase::connect()
    {
        std::string conn_info = "host=" + config.host +
                                " port=" + config.port +
                                " dbname=" + config.dbname +
                                " user=" + config.user +
                                " password=" + config.password +
                                " connect_timeout=" + std::to_string(config.connect_timeout);
        conn = PQconnectdb(conn_info.c_str());
        if (PQstatus(conn) != CONNECTION_OK)
        {
            spdlog::error("[NPUDatabase] Failed to connect to database");
            disconnect();
            return false;
        }
        spdlog::info("[NPUDatabase] Connected to database {}",config.dbname);
        return true;
    }

    void NPUDatabase::disconnect()
    {
        if (conn)
        {
            PQfinish(conn);
            conn = nullptr;
            spdlog::info("[NPUDatabase] Disconnected from database");
        }
    }

    bool NPUDatabase::createMetricTable()
    {
        const std::string sql =
            "CREATE TABLE IF NOT EXISTS " + metric_table_name + " ("
            "timestamp TIMESTAMP NOT NULL,"
            "card_id INTEGER NOT NULL,"
            "device_id INTEGER NOT NULL,"
            "util_aicore INTEGER,"
            "util_aicpu INTEGER,"
            "util_mem INTEGER,"
            "util_membw INTEGER,"
            "util_vec INTEGER,"
            "freq_aicore INTEGER,"
            "freq_aicpu INTEGER,"
            "freq_mem INTEGER,"
            "power DOUBLE PRECISION,"
            "health INTEGER,"
            "temperature INTEGER,"
            "voltage DOUBLE PRECISION"
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

    bool NPUDatabase::execSQL(const std::string& sql, const std::vector<const char*>& params)
    {
        PGresult* res = nullptr;
        // 执行语句
        if (params.empty())res = PQexec(conn, sql.c_str());
        else
        {
            res = PQexecParams(
                conn,
                sql.c_str(),
                static_cast<int>(params.size()),
                nullptr,
                params.data(),
                nullptr,
                nullptr,
                0
            );
        }
        // 判断执行结果，确定是否回滚
        if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            spdlog::warn("[NPUDatabase] SQL execution failed: {}", PQerrorMessage(conn));
            PQclear(res);
            return false;
        }
        PQclear(res);
        return true;
    }

    bool NPUDatabase::startTransaction()
    {
        PGresult*res = PQexec(conn, "BEGIN");
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            spdlog::warn("[NPUDatabase] Failed to begin transaction for static info: {}", 
                        std::string(PQerrorMessage(conn)));
            PQclear(res);
            return false;
        }
        PQclear(res);
        return true;
    }

    bool NPUDatabase::commitTransaction()
    {
        PGresult*res = PQexec(conn, "COMMIT");
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            spdlog::warn("[NPUDatabase] Failed to commit transaction for static info: {}", 
                        std::string(PQerrorMessage(conn)));
            PQclear(res);
            return false;
        }
        PQclear(res);
        return true;
    }
}

#endif