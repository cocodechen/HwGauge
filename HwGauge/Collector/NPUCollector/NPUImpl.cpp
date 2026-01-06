#ifdef HWGAUGE_USE_NPU

#include <assert.h>
#include <iostream>
#include <dcmi_interface_api.h>

#include "NPUImpl.hpp"

#define NPU_OK (0)

namespace hwgauge
{
    NPUImpl::NPUImpl()
    {
        int ret = dcmi_init();
        if(ret!=NPU_OK)raise_error("dcmi_init failed",ret,-1,-1,true);
        is_label_initialized=false;
    }

    std::string NPUImpl::name() const
    {
        return "ascend_npu";
    }

    std::vector<NPULabel> NPUImpl::labels()
    {
        if(is_label_initialized)return label_list;
        is_label_initialized=true;
        //获取卡列表
        int ret;
        int card_count = 0;
        int card_list[MAX_CARD_NUM] = {0};
        ret=dcmi_get_card_list(&card_count, card_list, MAX_CARD_NUM);
        if(ret!=NPU_OK)raise_error("dcmi_get_card_list failed",ret,-1,-1,true);
        
        assert(card_count!=0);

        //遍历每个卡和设备
        for (int i = 0; i < card_count; i++)
        {
            int card = card_list[i];
            int device_count = 0, mcu_id = 0, cpu_id = 0;
            ret=dcmi_get_device_id_in_card(card, &device_count, &mcu_id, &cpu_id);

            if(ret!=NPU_OK)
            {
                raise_error("dcmi_get_device_id_in_card failed",ret,card,-1,true);
                continue;
            }

            for (int dev = 0; dev < device_count; dev++)
            {
                NPULabel label;
                label.card_id = card;
                label.device_id = dev;
                label_list.push_back(label);
            }
        }
        return label_list;
    }
        
    /*采集所有设备的指标数据*/
    std::vector<NPUMetric> NPUImpl::sample()
    {
        if(!is_label_initialized)raise_error("label hasn't been called",-1,-1,-1,true);
        std::vector<NPUMetric> metrics;
        //为每个设备采集数据
        for (const auto& label : label_list)
        {
            NPUMetric metric;
            collect_single_device(label.card_id, label.device_id, metric);
            metrics.push_back(metric);
        }
        return metrics;
    }

    /*采集单个设备的指标*/
    void NPUImpl::collect_single_device(int card, int device, NPUMetric& metric)
    {
        int ret;
        /*利用率*/
        unsigned int util_aicore = 0, util_aicpu = 0, util_mem = 0;
        //AICore
        ret=dcmi_get_device_utilization_rate(card, device, 2, &util_aicore);
        if (ret== NPU_OK)metric.util_aicore = util_aicore;
        else
        {
            metric.util_aicore = 0;
            raise_error("get AICore utilization rate failed",ret,card,device,false);
        }
        //AICPU
        ret=dcmi_get_device_utilization_rate(card, device, 3, &util_aicpu);
        if (ret== NPU_OK)metric.util_aicpu = util_aicpu;
        else
        {
            metric.util_aicpu = 0;
            raise_error("get AICPU utilization rate failed",ret,card,device,false);
        }
        //Mem
        ret=dcmi_get_device_utilization_rate(card, device, 1, &util_mem);
        if (ret== NPU_OK)metric.util_mem = util_mem;
        else
        {
            metric.util_mem = 0;
            raise_error("get Mem utilization rate failed",ret,card,device,false);
        }

        /*频率*/
        //AICore
        struct dcmi_aicore_info aicore = {0};
        ret=dcmi_get_device_aicore_info(card, device, &aicore);
        if (ret== NPU_OK)metric.aicore_freq = aicore.cur_freq;
        else
        {
            metric.aicore_freq = 0;
            raise_error("get AICore Frequency failed",ret,card,device,false);
        }
        //AICPU
        struct dcmi_aicpu_info aicpu = {0};
        ret=dcmi_get_device_aicpu_info(card, device, &aicpu);
        if (ret == NPU_OK)metric.aicpu_freq = aicpu.cur_freq;
        else
        {
            metric.aicpu_freq = 0;
            raise_error("get AICPU Frequency failed",ret,card,device,false);
        }
        //Mem
        unsigned int mem_freq;
        ret=dcmi_get_device_frequency(card,device,(enum dcmi_freq_type)1,&mem_freq);
        if(ret==NPU_OK)metric.mem_freq=mem_freq;
        else
        {
            metric.mem_freq=0;
            raise_error("get Mem Frequency failed",ret,card,device,false);
        }

        /*功耗*/
        int power = 0;
        ret=dcmi_get_device_power_info(card, device, &power);
        if (ret == NPU_OK)metric.power = (double)power/10.0;
        else
        {
            metric.power = 0;
            raise_error("get power failed",ret,card,device,false);
        }

        /*其他*/
        //健康状态
        unsigned int health = 0;
        ret=dcmi_get_device_health(card, device, &health);
        if (ret == NPU_OK)metric.health = health;
        else
        {
            metric.health = 0xFFFFFFFF;
            raise_error("get health failed",ret,card,device,false);
        }
        
        //温度
        int temperature = 0;
        ret=dcmi_get_device_temperature(card, device, &temperature);
        if (ret == NPU_OK)metric.temperature = temperature;
        else
        {
            metric.temperature = 0;
            raise_error("get temperature failed",ret,card,device,false);
        }
        
        //电压
        unsigned int voltage = 0;
        ret=dcmi_get_device_voltage(card, device, &voltage);
        if (ret == NPU_OK)metric.voltage = (double)voltage/100.0;
        else
        {
            metric.voltage = 0;
            raise_error("get voltage failed",ret,card,device,false);
        }
    }

    /*错误信息*/
    void NPUImpl::raise_error(const std::string&msg, int ret,int card,int dev,bool fatal)
    {
        std::string out;
        out += fatal ? "[FATAL] " : "[WARNING] ";
        out += msg;
        out += "(ret=" + std::to_string(ret)+") ";
        if (card >= 0)out += "(card=" + std::to_string(card)+") ";
        if (dev >= 0)out += "(dev=" + std::to_string(dev)+") ";
        std::cerr << out << std::endl;
        if (fatal)std::exit(EXIT_FAILURE);
    }
}

#endif