# NPU能耗监测数据库设计文档
## 1.设计目标
为 NPU 芯片级能耗与运行状态提供**持续监测的数据支撑**，服务于：
- 耗建模
- 动态调度策略研究

## 2.设计原则
- 以设备为单位建表
- 为每个设备建静态信息和动态信息分别建一张表


## 3.数据库表结构
### 3.1 芯片静态信息表
记录芯片固有属性，数据不随时间变化

```sql
npu_chip_info (
    card_id     INT,
    device_id   INT,
    chip_name   TEXT,
    chip_type   TEXT,
    PRIMARY KEY (card_id, device_id)
);
```

**字段说明：**

|字段名	|含义|
|---|---|
|card_id | 设备中的卡编号|
|device_id	|卡上的芯片编号|
|chip_name	|芯片名称|
|chip_type	|芯片类型|

### 3.2芯片动态监测表
记录芯片运行时状态与能耗的时间序列数据

```sql
npu_chip_metrics (
    ts_ns        BIGINT,
    card_id      INT,
    device_id    INT,
    
    util_aicore  INT,
    util_aicpu   INT,
    util_mem     INT,
    util_membw   INT,
    util_vec     INT,
    
    freq_aicore  INT,
    freq_aicpu   INT,
    freq_mem     INT,
    
    power        DOUBLE PRECISION,
    health       INT,
    temperature  INT,
    voltage      DOUBLE PRECISION,
    
    PRIMARY KEY (ts_ns, card_id, device_id)
);
```

**字段说明:**
|字段	|含义	|单位|
|---|---|---|
ts_ns|	采样时间戳	|-
|card_id | 设备中的卡编号|-
|device_id	|卡上的芯片编号|-
util_aicore|	AI核心利用率|	%
util_aicpu	|AI CPU利用率|	%
util_mem	|内存利用率	|%
util_membw	|内存带宽利用率	|%
util_vec	|矢量单元利用率|	%
freq_aicore	|AI核心频率	|MHz
freq_aicpu	|AI CPU频率|	MHz
freq_mem	|内存频率|	MHz
power	|芯片功耗	|W
health	|芯片健康状态	|-
temperature|	芯片温度|	℃
voltage	|工作电压|	V

**说明：当芯片不支持某指标或采样无法获取时，对应字段存为 NULL**



