#ifdef HWGAUGE_USE_NPU

#include <assert.h>
#include <iostream>
#include <dcmi_interface_api.h>
#include "spdlog/spdlog.h"

#include "Collector/Common/Exception.hpp"
#include "NPUImpl.hpp"

#define NPU_OK (0)

namespace hwgauge
{
    NPUImpl::NPUImpl()
    {
        int ret = dcmi_init();
        if(ret!=NPU_OK)throw hwgauge::FatalError("[NPUImpl] dcmi_init failed");
    }

    std::string NPUImpl::name() const
    {
        return "ascend_npu";
    }

    std::vector<NPULabel> NPUImpl::labels()
    {
        //获取卡列表
        int ret;
        int card_count = 0;
        int card_list[MAX_CARD_NUM] = {0};
        ret=dcmi_get_card_list(&card_count, card_list, MAX_CARD_NUM);
        if(ret!=NPU_OK)throw hwgauge::FatalError("[NPUImpl] dcmi_get_card_list failed");
        if(card_count==0)throw hwgauge::FatalError("[NPUImpl] The card of num is zero");

        //遍历每个卡和设备
        std::vector<NPULabel> labels;
        NPULabel label;
        for (int i = 0; i < card_count; i++)
        {
            int card = card_list[i];
            int device_count = 0, mcu_id = 0, cpu_id = 0;
            ret=dcmi_get_device_id_in_card(card, &device_count, &mcu_id, &cpu_id);
            if(ret!=NPU_OK)
            {
                spdlog::warn("[NPUImpl] dcmi_get_device_id_in_card failed (ret={}, card={})",ret,card);
                continue;
            }

            for (int dev = 0; dev < device_count; dev++)
            {
                label.card_id = card;
                label.device_id = dev;
                struct dcmi_chip_info chip_info = {0};
                ret = dcmi_get_device_chip_info(card, dev, &chip_info);
                if (ret == NPU_OK)
                {
                    label.chip_type.assign(
                        reinterpret_cast<const char*>(chip_info.chip_type),
                        strnlen(reinterpret_cast<const char*>(chip_info.chip_type), sizeof(chip_info.chip_type))
                    );
                    label.chip_name.assign(
                        reinterpret_cast<const char*>(chip_info.chip_name),
                        strnlen(reinterpret_cast<const char*>(chip_info.chip_name), sizeof(chip_info.chip_name))
                    );
                }
                else
                {
                    label.chip_type="";
                    label.chip_name="";
                    spdlog::warn("[NPUImpl] Get npu info failed (ret={}, card={}, device={})",ret,card,dev);
                }
                labels.push_back(label);
            }
        }
        return labels;
    }
        
    std::vector<NPUMetrics> NPUImpl::sample(std::vector<NPULabel>&labels)
    {
        std::vector<NPUMetrics> metrics;
        NPUMetrics metric;
        //为每个设备采集数据
        for (const auto& label : labels)
        {
            collect_single_device_metric(label.card_id, label.device_id, metric);
            metrics.push_back(metric);
        }
        return metrics;
    }

    void NPUImpl::collect_single_device_metric(int card, int device, NPUMetrics& metric)
    {
        int ret;
        /*频率*/
        //AICore
        struct dcmi_aicore_info aicore = {0};
        ret=dcmi_get_device_aicore_info(card, device, &aicore);
        if (ret== NPU_OK)metric.freq_aicore = aicore.cur_freq;
        else
        {
            metric.freq_aicore = -1;
            spdlog::warn("[NPUImpl] Get AICore Frequency failed (ret={}, card={}, device={})",ret,card,device);
        }
        //AICPU
        struct dcmi_aicpu_info aicpu = {0};
        ret=dcmi_get_device_aicpu_info(card, device, &aicpu);
        if (ret == NPU_OK)metric.freq_aicpu = aicpu.cur_freq;
        else
        {
            metric.freq_aicpu = -1;
            spdlog::warn("[NPUImpl] Get AICPU Frequency failed (ret={}, card={}, device={})",ret,card,device);
        }
        //CtrlCPU
        unsigned int freq_ctrlcpu=0;
        ret= dcmi_get_device_frequency(card,device,2, &freq_ctrlcpu);
        if (ret== NPU_OK)metric.freq_ctrlcpu = freq_ctrlcpu;
        else
        {
            metric.freq_ctrlcpu = -1;
            spdlog::warn("[NPUImpl] Get CtrlCPU Frequency rate failed (ret={}, card={}, device={})",ret,card,device);
        }


        /*利用率*/
        unsigned int util_aicore = 0;
        //AICore
        ret=dcmi_get_device_utilization_rate(card, device, 2, &util_aicore);
        if (ret== NPU_OK)metric.util_aicore = util_aicore;
        else
        {
            metric.util_aicore = -1;
            spdlog::warn("[NPUImpl] Get AICore utilization rate failed (ret={}, card={}, device={})",ret,card,device);
        }
        //AICPU
        unsigned int util_aicpu = 0;
        ret=dcmi_get_device_utilization_rate(card, device, 3, &util_aicpu);
        if (ret== NPU_OK)metric.util_aicpu = util_aicpu;
        else
        {
            metric.util_aicpu = -1;
            spdlog::warn("[NPUImpl] Get AICPU utilization rate failed (ret={}, card={}, device={})",ret,card,device);
        }
        //CtrlCPU
        unsigned int util_ctrlcpu=0;
        ret=dcmi_get_device_utilization_rate(card, device, 4, &util_ctrlcpu);
        if (ret== NPU_OK)metric.util_ctrlcpu = util_ctrlcpu;
        else
        {
            metric.util_ctrlcpu = -1;
            spdlog::warn("[NPUImpl] Get CtrlCPU utilization rate failed (ret={}, card={}, device={})",ret,card,device);
        }
        //vector core
        unsigned int util_vec=0;
        ret=dcmi_get_device_utilization_rate(card, device, 12, &util_vec);
        if (ret== NPU_OK)metric.util_vec = util_vec;
        else
        {
            metric.util_vec = -1;
            spdlog::warn("[NPUImpl] Get vector core utilization rate failed (ret={}, card={}, device={})",ret,card,device);
        }

        /*显存*/
        struct dcmi_hbm_info hbm_info = {0};
        ret = dcmi_get_device_hbm_info(card, device, &hbm_info);
        if (ret == NPU_OK)
        {
            metric.mem_total_mb = (unsigned long)hbm_info.memory_size; // MB
            metric.mem_usage_mb = (unsigned long)hbm_info.memory_usage;
            if(metric.mem_total_mb<metric.mem_usage_mb)throw RecoverableError("[NPUImpl] The memory total mb is larger than used mb");
            metric.util_mem=(int)(((double)metric.mem_usage_mb/metric.mem_total_mb)*100.0);
            metric.util_membw = hbm_info.bandwith_util_rate;
            metric.freq_mem=hbm_info.freq;
        }
        else
        {
            metric.mem_total_mb = -1;
            metric.mem_usage_mb = -1;
            metric.util_mem=-1;
            metric.util_membw = -1;
            metric.freq_mem=-1;
            spdlog::warn("[NPUImpl] Get On-chip memory failed (ret={}, card={}, device={})",ret,card,device);
        }

        /*功耗*/
        int power = 0;
        ret=dcmi_get_device_power_info(card, device, &power);
        if (ret == NPU_OK)metric.chip_power = (double)power/10.0;
        else
        {
            metric.chip_power = -1;
            spdlog::warn("[NPUImpl] Get chip power failed (ret={}, card={}, device={})",ret,card,device);
        }

        /*环境*/
        //健康状态
        unsigned int health = 0;
        ret=dcmi_get_device_health(card, device, &health);
        if (ret == NPU_OK)metric.health = health;
        else
        {
            metric.health = 0xFFFFFFFF;
            spdlog::warn("[NPUImpl] Get health failed (ret={}, card={}, device={})",ret,card,device);
        }
        
        //温度
        int temperature = 0;
        ret=dcmi_get_device_temperature(card, device, &temperature);
        if (ret == NPU_OK)metric.temperature = temperature;
        else
        {
            metric.temperature = -1;
            spdlog::warn("[NPUImpl] Get temperature failed (ret={}, card={}, device={})",ret,card,device);
        }
        
        //电压
        unsigned int voltage = 0;
        ret=dcmi_get_device_voltage(card, device, &voltage);
        if (ret == NPU_OK)metric.voltage = (double)voltage/100.0;
        else
        {
            metric.voltage = -1;
            spdlog::warn("[NPUImpl] Get voltage failed (ret={}, card={}, device={})",ret,card,device);
        }
    }

}

#endif