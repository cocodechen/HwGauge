#pragma once

#ifdef HWGAUGE_USE_NPU

#include <vector>
#include "NPUMetrics.hpp"

namespace hwgauge
{
    /*底层信息采集*/
    class NPUImpl
    {
    public:
        /*构造函数：初始化DCMI*/
        NPUImpl();
        
        /*返回收集器名称*/
        std::string name() const;
        
        /*返回所有设备的标签*/
        std::vector<NPULabel> labels();

        /*采集所有设备的指标数据*/
        std::vector<NPUMetrics> sample();

        /*采集所有设备的静态数据*/
        std::vector<NPUInfo> getInfo();
        
    private:
        /*唯一的标签*/
        std::vector<NPULabel> label_list;
        bool is_label_initialized;
        /*唯一的静态信息*/
        std::vector<NPUInfo> info_list;
        bool is_info_got;

        /*采集单个设备的指标数据*/
        void collect_single_device_metric(int card, int device, NPUMetrics& metric);

        /*采集单个设备的静态数据*/
        void collect_single_device_info(int card, int device, NPUInfo& info);
    };
}

#endif
