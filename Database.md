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
    timestamp TIMESTAMP NOT NULL,              -- Sampling timestamp
    cpu_index INTEGER NOT NULL,                -- CPU/socket index
    
    cpu_utilization DOUBLE PRECISION,          -- CPU utilization (%)
    cpu_frequency DOUBLE PRECISION,            -- CPU frequency (MHz)
    c0_residency DOUBLE PRECISION,             -- C0 state residency (%)
    c6_residency DOUBLE PRECISION,             -- C6 state residency (%)
    power_usage DOUBLE PRECISION,              -- Power usage (W)
    temperature DOUBLE PRECISION,              -- Temperature (℃) [NEW]
    
    memory_read_bandwidth DOUBLE PRECISION,    -- Memory read bandwidth (MB/s)
    memory_write_bandwidth DOUBLE PRECISION,   -- Memory write bandwidth (MB/s)
    memory_power_usage DOUBLE PRECISION,       -- Memory power usage (W)
    
    PRIMARY KEY (timestamp, cpu_index)
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
    timestamp TIMESTAMP NOT NULL,              -- Sampling timestamp
    gpu_index INTEGER NOT NULL,                -- GPU index
    
    gpu_utilization DOUBLE PRECISION,          -- GPU core utilization (%)
    memory_utilization DOUBLE PRECISION,       -- VRAM utilization (%)
    gpu_frequency DOUBLE PRECISION,            -- Core frequency (MHz)
    memory_frequency DOUBLE PRECISION,         -- Memory frequency (MHz)
    power_usage DOUBLE PRECISION,              -- Power usage (W)
    temperature DOUBLE PRECISION,              -- Temperature (℃) [NEW]
    
    PRIMARY KEY (timestamp, gpu_index)
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
    timestamp TIMESTAMP NOT NULL,      -- Sampling timestamp
    card_id   INTEGER NOT NULL,        -- Card ID
    device_id INTEGER NOT NULL,        -- Chip ID
    
    -- Utilization Metrics (%)
    util_aicore  INTEGER,              -- AI Core Utilization
    util_aicpu   INTEGER,              -- AI CPU Utilization
    util_ctrlcpu INTEGER,              -- Control CPU Utilization [NEW]
    util_vec     INTEGER,              -- Vector Core Utilization
    util_mem     INTEGER,              -- Memory Utilization (%)
    util_membw   INTEGER,              -- Memory Bandwidth Utilization
    
    -- Frequency Metrics (MHz)
    freq_aicore  INTEGER,              -- AI Core Frequency
    freq_aicpu   INTEGER,              -- AI CPU Frequency
    freq_ctrlcpu INTEGER,              -- Control CPU Frequency [NEW]
    freq_mem     INTEGER,              -- Memory Frequency
    
    -- Memory Capacity (MB) [NEW]
    mem_total_mb BIGINT,               -- Total Memory (MB)
    mem_used_mb  BIGINT,               -- Used Memory (MB)

    -- Power & Status
    power        DOUBLE PRECISION,     -- Chip Power (W)
    health       INTEGER,              -- Health Status (0:OK, 1:WARN, 2:ERR, 3:CRIT)
    temperature  INTEGER,              -- Chip Temperature (℃)
    voltage      DOUBLE PRECISION,     -- Input Voltage (V)
    
    PRIMARY KEY (timestamp, card_id, device_id),
    FOREIGN KEY (card_id, device_id) REFERENCES hwgauge_npu_chip_info(card_id, device_id)
);
```

#### 4. 系统整体监控表

**说明：系统级指标为全局信息，不需要独立的静态信息表。**

**系统动态监测表**:
```sql
CREATE TABLE IF NOT EXISTS hwgauge_system_metrics (
    timestamp TIMESTAMP NOT NULL,              -- 采样时间戳
    
    -- 内存指标
    mem_total_gb DOUBLE PRECISION,             -- 物理内存总量(GB)
    mem_used_gb DOUBLE PRECISION,              -- 已用物理内存(GB)
    mem_util_percent DOUBLE PRECISION,         -- 内存利用率(%)
    
    -- 磁盘IO指标 (汇总)
    disk_read_mbps DOUBLE PRECISION,           -- 磁盘读吞吐量(MB/s)
    disk_write_mbps DOUBLE PRECISION,          -- 磁盘写吞吐量(MB/s)
    max_disk_util_percent DOUBLE PRECISION,    -- 最忙碌磁盘利用率(%) - 标识系统IO瓶颈
    
    -- 网络指标 (汇总)
    net_download_mbps DOUBLE PRECISION,        -- 网络下载带宽(MB/s)
    net_upload_mbps DOUBLE PRECISION,          -- 网络上传带宽(MB/s)
    
    -- 整机功耗
    system_power_watts DOUBLE PRECISION,       -- 系统整机功耗(W)
    
    PRIMARY KEY (timestamp)
);
```
