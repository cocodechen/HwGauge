#pragma once

#ifdef HWGAUGE_USE_NPU

#include <string>
#include <vector>
#include "NPUCollector.hpp"

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
        std::vector<NPUMetric> sample();
        
    private:
        /*唯一的标签*/
        std::vector<NPULabel> label_list;
        bool is_label_initialized;

        /*采集单个设备的指标*/
        void collect_single_device(int card, int device, NPUMetric& metric);
        /*错误信息*/
        void raise_error(const std::string&msg, int ret,int card,int dev,bool fatal);
    };
}

#endif
