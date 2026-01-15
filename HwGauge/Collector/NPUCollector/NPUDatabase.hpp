#pragma once
#if defined(HWGAUGE_USE_POSTGRESQL) && defined(HWGAUGE_USE_NPU)

#include "NPUMetrics.hpp"
#include "Collector/DBConfig.hpp"

#include <libpq-fe.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>   
#include <iomanip> 

namespace hwgauge
{
    /*NPU数据数据库操作类（使用libpq C API）*/
    class NPUDatabase
    {
    public:
        /*构造函数：创建数据库，建立连接*/
        explicit NPUDatabase(const ConnectionConfig& config_, const std::string table_name_);
        
        /*析构函数，断开连接*/
        ~NPUDatabase();
        
        /*禁用拷贝*/
        NPUDatabase(const NPUDatabase&) = delete;
        NPUDatabase& operator=(const NPUDatabase&) = delete;
        
        /*检查是否已连接*/
        bool isConnected() const;
        
        /*写入NPU监控数据*/
        void writeMetric(const std::vector<NPULabel>& label_list, 
                        const std::vector<NPUMetrics>& metric_list,
                        bool useTransaction = true);
        /*写入NPU静态数据*/
        void writeInfo(const std::vector<NPULabel>& label_list, 
                    const std::vector<NPUInfo>& metric_list,
                    bool useTransaction = true);
        
    private:
        PGconn* conn;               // PostgreSQL连接对象指针
        ConnectionConfig config;    // 连接配置
        std::string metric_table_name;      // 指标数据表名
        std::string info_table_name;        // 静态数据表名
        std::string metric_insert_sql;      // 指标数据插入语句
        std::string info_insert_sql;        // 静态数据插入语句

        /*连接到数据库*/
        bool connect();
        /*断开数据库连接*/
        void disconnect();

        /*创建指标数据表*/
        bool createMetricTable();
        /*创建静态数据表*/
        bool createInfoTable();

        // 公共执行函数，支持参数化
        bool execSQL(const std::string& sql, const std::vector<const char*>& params = {});

        /*开始事务*/
        bool startTransaction();
        /*提交事务*/
        bool commitTransaction();

        /*将数值转为存入数据库的格式，如果数值不可能为NULL，可以不用*/
        inline const char* to_sql_param_int(int value, std::string& buf)
        {
            if (value == -1) return nullptr;  // SQL NULL
            buf = std::to_string(value); // 存储字符串，保证生命周期
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
            if (value.empty())return nullptr;
            buf = value;
            return buf.c_str();
        }

        /*获取当前时间*/
        inline std::string getNowTime()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_time);
            std::ostringstream timestamp_ss;
            timestamp_ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
            std::string timestamp = timestamp_ss.str();
            return timestamp;
        }

    };
}

#endif