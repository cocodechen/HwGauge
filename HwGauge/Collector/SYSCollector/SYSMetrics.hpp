#pragma once

#ifdef __linux__

#include <string>

namespace hwgauge
{
    struct SYSLabel
    {
        std::string name; // 例如 "System-Global"
    };

    struct SYSMetrics
    {
        // 内存
        double memTotalGB;
        double memUsedGB;
        double memUtilizationPercent;

        // 磁盘 (汇总所有物理盘)
        double diskReadMBps;      // 读吞吐量 MB/s
        double diskWriteMBps;     // 写吞吐量 MB/s
        double maxDiskUtilPercent; // 最忙碌的那块盘的利用率 %，木桶效应说是

        // 网络 (汇总所有物理网卡)
        double netDownloadMBps;   // 下载带宽 MB/s
        double netUploadMBps;     // 上传带宽 MB/s

        // 功耗
        double systemPowerWatts;  // 整机功耗 (W)

        SYSMetrics() 
            : memTotalGB(-1.0), memUsedGB(-1.0), memUtilizationPercent(-1.0),
              diskReadMBps(-1.0), diskWriteMBps(-1.0), maxDiskUtilPercent(-1.0),
              netDownloadMBps(-1.0), netUploadMBps(-1.0),
              systemPowerWatts(-1.0) 
        {}
    };
}

#endif