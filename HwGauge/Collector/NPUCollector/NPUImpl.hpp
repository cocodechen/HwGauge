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

        /*采集标签label的指标数据*/
        std::vector<NPUMetrics> sample(std::vector<NPULabel>&labels);
        
    private:
        /*采集单个设备的指标数据*/
        void collect_single_device_metric(int card, int device, NPUMetrics& metric);
    };
}

#endif
