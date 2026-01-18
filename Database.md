## 🗄️ PostgreSQL Storage

### 数据库架构设计

HwGauge采用**静态信息表**(```[your_table_name]_info```)和**动态监测表**(`[your_table_name]_metrics`)分离的设计，提供高效的数据存储和查询：

**说明：当芯片不支持某指标或采样无法获取时，对应字段存为 NULL**

#### 1. CPU监控表

**CPU静态信息表**:
```sql
CREATE TABLE IF NOT EXISTS hwgauge_cpu_info (
    cpu_index INTEGER NOT NULL PRIMARY KEY,  -- CPU/socket索引
    cpu_name TEXT                            -- CPU名称
);
```

**CPU动态监测表**:
```sql
CREATE TABLE IF NOT EXISTS hwgauge_cpu_metrics (
    timestamp TIMESTAMP NOT NULL,              -- 采样时间戳
    cpu_index INTEGER NOT NULL,                -- CPU/socket索引
    
    cpu_utilization DOUBLE PRECISION,          -- CPU利用率(%)
    cpu_frequency DOUBLE PRECISION,            -- CPU频率(MHz)
    c0_residency DOUBLE PRECISION,             -- C0状态占比(%)
    c6_residency DOUBLE PRECISION,             -- C6状态占比(%)
    power_usage DOUBLE PRECISION,              -- 功耗(W)
    
    memory_read_bandwidth DOUBLE PRECISION,    -- 内存读带宽(MB/s)
    memory_write_bandwidth DOUBLE PRECISION,   -- 内存写带宽(MB/s)
    memory_power_usage DOUBLE PRECISION,       -- 内存功耗(W)
    
    PRIMARY KEY (timestamp, cpu_index),
);
```

#### 2. GPU监控表
**GPU静态信息表**:

```sql
CREATE TABLE IF NOT EXISTS hwgauge_gpu_info (
    gpu_index INTEGER NOT NULL PRIMARY KEY,    -- GPU索引
    gpu_name TEXT                              -- GPU名称
);
```


**GPU动态监测表**:

```sql
CREATE TABLE IF NOT EXISTS hwgauge_gpu_metrics (
    timestamp TIMESTAMP NOT NULL,              -- 采样时间戳
    gpu_index INTEGER NOT NULL,                -- GPU索引
    
    gpu_utilization DOUBLE PRECISION,          -- GPU利用率(%)
    memory_utilization DOUBLE PRECISION,       -- 显存利用率(%)
    gpu_frequency DOUBLE PRECISION,            -- GPU频率(MHz)
    memory_frequency DOUBLE PRECISION,         -- 显存频率(MHz)
    power_usage DOUBLE PRECISION,              -- 功耗(W)
    
    PRIMARY KEY (timestamp, gpu_index),
);
```

#### 3. NPU监控表

**NPU芯片静态信息表**:

```sql
CREATE TABLE IF NOT EXISTS hwgauge_npu_chip_info (
    card_id   INTEGER NOT NULL,    -- 设备中的卡编号
    device_id INTEGER NOT NULL,    -- 卡上的芯片编号
    chip_name TEXT,                -- 芯片名称
    chip_type TEXT,                -- 芯片类型
    
    PRIMARY KEY (card_id, device_id)
);
```

**NPU芯片动态监测表**:
```sql
CREATE TABLE IF NOT EXISTS hwgauge_npu_chip_metrics (
    timestamp TIMESTAMP NOT NULL,      -- 采样时间戳
    card_id   INTEGER NOT NULL,        -- 设备中的卡编号
    device_id INTEGER NOT NULL,        -- 卡上的芯片编号
    
    -- 利用率指标 (%)
    util_aicore  INTEGER,              -- AI核心利用率
    util_aicpu   INTEGER,              -- AI CPU利用率
    util_mem     INTEGER,              -- 内存利用率
    util_membw   INTEGER,              -- 内存带宽利用率
    util_vec     INTEGER,              -- 矢量单元利用率
    
    -- 频率指标 (MHz)
    freq_aicore  INTEGER,              -- AI核心频率
    freq_aicpu   INTEGER,              -- AI CPU频率
    freq_mem     INTEGER,              -- 内存频率
    
    -- 能耗与状态指标
    power        DOUBLE PRECISION,     -- 芯片功耗(W)
    health       INTEGER,              -- 芯片健康状态(0:OK,1:WARN,2:ERROR,3:CRITICAL,0xFFFFFFFF:NOT_EXIST)
    temperature  INTEGER,              -- 芯片温度(℃)
    voltage      DOUBLE PRECISION,     -- 工作电压(V)
    
    PRIMARY KEY (timestamp, card_id, device_id),
    FOREIGN KEY (card_id, device_id) REFERENCES hwgauge_npu_chip_info(card_id, device_id)
);
```