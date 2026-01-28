#pragma once

#include <string>

#ifdef HWGAUGE_USE_PROMETHEUS
#include <prometheus/registry.h>
#endif

namespace hwgauge
{
#ifdef HWGAUGE_USE_POSTGRESQL
    /*数据库连接配置结构体*/
    struct ConnectionConfig
    {
        std::string host;
        std::string port;
        std::string dbname;
        std::string user;
        std::string password;
        int connect_timeout;

        // 默认构造函数
        ConnectionConfig()
            : host("localhost"),
            port("5432"),
            dbname("postgres"),
            user("postgres"),
            password("123456"),
            connect_timeout(5)
        {}

        // 参数构造函数
        ConnectionConfig(std::string host_,
                        std::string port_,
                        std::string dbname_,
                        std::string user_,
                        std::string password_,
                        int connect_timeout_)
            : host(std::move(host_)),
            port(std::move(port_)),
            dbname(std::move(dbname_)),
            user(std::move(user_)),
            password(std::move(password_)),
            connect_timeout(connect_timeout_)
        {}
    };
#endif

    /*Collector配置*/
    struct CollectorConfig
    {
        bool outTer=true;
        bool outFile=false;
        std::string filepath;
#ifdef HWGAUGE_USE_PROMETHEUS
        bool pmEnable = false;
        std::shared_ptr<prometheus::Registry> registry;
#endif
#ifdef HWGAUGE_USE_POSTGRESQL
        bool dbEnable = false;
        ConnectionConfig dbConfig;
        std::string dbTableName;
#endif
    };
}