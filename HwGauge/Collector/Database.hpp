#pragma once

#ifdef HWGAUGE_USE_POSTGRESQL

#include "Collector/Config.hpp"
#include "Collector/Exception.hpp"
#include "spdlog/spdlog.h"

#include <libpq-fe.h>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>
#include <memory>

namespace hwgauge
{
    /* 数据库基类 （使用libpq C API）*/
    template<typename LabelType, typename MetricsType>
    class Database
    {
        public:
            /* 构造函数：建立连接 */
            explicit Database(const ConnectionConfig& config_)
                : conn(nullptr), config(config_)
            {
                // 建立连接
                if (!connect())throw hwgauge::FatalError("[Database] Connecect Failed");
            }
            
            /* 析构函数，断开连接 */
            virtual ~Database()
            {
                disconnect();
            }
            
            /* 禁用拷贝 */
            Database(const Database&) = delete;
            Database& operator=(const Database&) = delete;
            
            /* 检查是否已连接 */
            bool isConnected() const
            {
                if(conn != nullptr && PQstatus(conn) == CONNECTION_OK)return true;
                else
                {
                    spdlog::warn("[Database] Database has not connected");
                    return false;
                }
            }
            
            /* 写入指标数据 - 纯虚函数，子类必须实现 */
            virtual void writeMetric(const std::string&cur_time,
                                    const std::vector<LabelType>& label_list,
                                    const std::vector<MetricsType>& metric_list,
                                    bool useTransaction = true) = 0;
            
            /* 写入静态信息 - 纯虚函数，子类必须实现 */
            virtual void writeInfo(const std::vector<LabelType>& label_list,
                                bool useTransaction = true)=0;
            
        protected:
            PGconn* conn;               // PostgreSQL连接对象指针
            ConnectionConfig config;    // 连接配置
            std::string metric_table_name;      // 指标数据表名
            std::string info_table_name;        // 静态数据表名
            std::string metric_insert_sql;      // 指标数据插入语句
            std::string info_insert_sql;        // 静态数据插入语句
            
            /* 连接到数据库 */
            bool connect()
            {
                std::string conn_info = "host=" + config.host +
                        " port=" + config.port +
                        " dbname=" + config.dbname +
                        " user=" + config.user;
                if (!config.password.empty())conn_info += " password=" + config.password;
                conn_info += " connect_timeout=" + std::to_string(config.connect_timeout);

                conn = PQconnectdb(conn_info.c_str());
                if (PQstatus(conn) != CONNECTION_OK)
                {
                    disconnect();
                    spdlog::warn("[Database] Failed to connect to database: {}",std::string(PQerrorMessage(conn)));
                    return false;
                }
                spdlog::info("[Database] Connected to database {}",config.dbname);
                return true;
            }
            /* 断开数据库连接 */
            void disconnect()
            {
                if (conn)
                {
                    PQfinish(conn);
                    conn = nullptr;
                    spdlog::info("[Database] Disconnected from database");
                }
            }
            
            /* 创建表 */
            virtual bool createMetricTable()=0;
            virtual bool createInfoTable()=0;

            /* 执行SQL语句 */
            bool execSQL(const std::string& sql, const std::vector<const char*>& params = {})
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
                // 判断执行结果
                if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    PQclear(res);
                    spdlog::warn("[Database] SQL execution failed: {}", std::string(PQerrorMessage(conn)));
                    return false;
                }
                PQclear(res);
                return true;
            }
            
            /* 开始事务 */
            bool startTransaction()
            {
                PGresult* res = PQexec(conn, "BEGIN");
                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    PQclear(res);
                    spdlog::warn("[Database] Failed to begin transaction for static info: {}", std::string(PQerrorMessage(conn)));
                    return false;
                }
                PQclear(res);
                return true;
            }
            /* 提交事务 */
            bool commitTransaction()
            {
                PGresult* res = PQexec(conn, "COMMIT");
                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    PQclear(res);
                    spdlog::warn("[Database] Failed to commit transaction for static info: {}", std::string(PQerrorMessage(conn)));
                    return false;
                }
                PQclear(res);
                return true;
            } 
            /* 回滚事务 */
            bool rollbackTransaction()
            {
                PGresult* res = PQexec(conn, "ROLLBACK");
                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    PQclear(res);
                    spdlog::warn("[Database] Failed to rollback transaction for static info: {}", std::string(PQerrorMessage(conn)));
                    return false;
                }
                PQclear(res);
                return true;
            }
            
            /* 参数转换辅助函数 */
            inline const char* to_sql_param_int(int value, std::string& buf)
            {
                if (value == -1) return nullptr;  // SQL NULL
                buf = std::to_string(value);
                return buf.c_str();
            } 
            inline const char* to_sql_param_long(long long value, std::string& buf)
            {
                if (value == -1) return nullptr;  // SQL NULL
                buf = std::to_string(value);
                return buf.c_str();
            }  
            inline const char* to_sql_param_double(double value, std::string& buf)
            {
                if (value == -1.0) return nullptr;
                buf = std::to_string(value);
                return buf.c_str();
            }
            inline const char* to_sql_param_string(const std::string& value, std::string& buf)
            {
                if (value.empty()) return nullptr;
                buf = value;
                return buf.c_str();
            }
        };
}

#endif