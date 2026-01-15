#pragma once
#include <string>

struct NPULabel
{
    int card_id;
    int device_id;
};

struct NPUMetrics
{
    //利用率（%）
    int util_aicore; //AICore
    int util_aicpu; //AICPU
    int util_mem; //Mem
    int util_membw; //内存带宽
    int util_vec; //vector core
    //频率（MHz）
    int freq_aicore; //AICore
    int freq_aicpu; //AICPU
    int freq_mem; //Mem
    //功耗（W）
    double power;

    //其他
    //健康状态 (0: OK, 1: WARN, 2: ERROR, 3: CRITICAL, 0xFFFFFFFF: NOT_EXIST)
    unsigned int health;
    //温度（C）
    int temperature;
    //电压（V）
    double voltage;
};

struct NPUInfo
{
    std::string chip_type; //芯片类型
    std::string chip_name; //芯片名称
};