# HwGauge

HwGauge is a lightweight hardware power-consumption exporter that exposes **CPU, GPU and NPU energy metrics as Prometheus Or PostgreSQL Gauges**.
It is implemented in modern C++ to provide **high-performance monitoring with minimal overhead**.


## ✨ Features

* 🖥️ **CPU Monitoring** — Intel PCM (Processor Counter Monitor)
* 🎮 **GPU Monitoring** — NVIDIA NVML (CUDA Toolkit required)
* 🧠 **NPU Monitoring** — Ascend NPU (DCMI required)
* 📊 **System Monitoring** — RAM, Disk I/O, Network, and Chassis Power (Linux /proc & sysfs)
* 📡 **Prometheus Exposer** — Built-in HTTP server with configurable endpoint
* 🗄️ **PostgreSQL Storage** — Store metrics in PostgreSQL for long-term retention
* ⚙️ **Template-based Collector Framework** — clean separation of metrics & hardware backends
* 🔌 **Unified Database Interface** — Support multiple storage backends with common API

---

## ⚙️ Architecture Diagram
```mermaid
flowchart TD
    %% 样式定义
    classDef hardware fill:#e1f5fe,stroke:#01579b,stroke-width:2px;
    classDef collector fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;
    classDef output fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px;
    classDef interface fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px,stroke-dasharray: 5 5;

    subgraph HardwareLayer ["HARDWARE INTERFACE LAYER"]
        direction TB
        A1["CPUImpl<br/>(Intel PCM)"]:::hardware
        A2["GPUImpl<br/>(NVIDIA NVML)"]:::hardware
        A3["NPUImpl<br/>(Ascend DCMI)"]:::hardware
        A4["SystemImpl<br/>(Sysfs/Procfs/IPMI)"]:::hardware
    end

    subgraph CollectorLayer ["COLLECTOR LAYER"]
        direction TB
        B1["CPUCollector"]:::collector
        B2["GPUCollector"]:::collector
        B3["NPUCollector"]:::collector
        B4["SYSCollector"]:::collector
    end

    subgraph OutputLayer ["OUTPUT LAYER"]
        direction TB
        %% 这是一个逻辑上的汇聚节点，使图表更简洁
        O_Proxy(("Unified Output<br/>Interface")):::interface
        
        C1["Prometheus<br/>Exporter"]:::output
        C2["PostgreSQL<br/>Storage"]:::output
        C3["Terminal<br/>Printer"]:::output
    end

    %% 核心控制器
    D{{Scheduler / Exposer}}:::interface

    %% 关系连线
    D -.-> B1 & B2 & B3 & B4

    A1 --> B1
    A2 --> B2
    A3 --> B3
    A4 --> B4

    %% 关键修改：所有收集器指向统一接口，再分发
    B1 & B2 & B3 & B4 ==> O_Proxy
    
    O_Proxy --> C1
    O_Proxy --> C2
    O_Proxy --> C3
```
---

## 📦 Prerequisites

| Requirement        | Notes                                                              |
| ------------------ | ------------------------------------------------------------------ |
| CMake ≥ 3.25       | Required for building                                              |
| C++17 compiler     | GCC / Clang / MSVC                                                 |
| CUDA Toolkit       | Required for NVML GPU monitoring                                   |
| NPU SDK/Driver     | Required for NPU monitoring                                        |
| prometheus-cpp     | Prometheus client development library(for Prometheus module)|
| libpq-dev          | PostgreSQL client development library (for SQL module)             |
| PostgreSQL Server  | PostgreSQL server for storing metrics (optional)                   |
| Sudo privileges    | Needed to access hardware registers (PCM)                          |
---

## 🔧 System Setup
To ensure full functionality of system metrics, specific kernel modules and tools are required.

### 1. CPU Temperature (`coretemp`)
Used to read CPU temperatures via `/sys/class/hwmon`.

```bash
# 1. Install kernel module extras (fixes modprobe errors on some distros)
sudo apt-get install linux-modules-extra-$(uname -r)
# 2. Load the module
sudo modprobe coretemp
# 3. Verify (should list temp inputs)
ls /sys/class/hwmon/hwmon*/temp*_input
```

### 2.System Power(`ipmitool or lm-sensors`)
Ensure your machine has a valid DCMI or a power-related sensor
```bash
# 1. Install tools
sudo apt-get update
sudo apt-get install ipmitool lm-sensors
# 2. Load IPMI modules (if not auto-loaded)
sudo modprobe ipmi_devintf
sudo modprobe ipmi_si
# 3. Power Reading Verification
sudo ipmitool dcmi power reading #Standard DCMI (Recommended):
sudo ipmitool sdr elist | grep -iE "Pwr|Power" #Legacy/Vendor Specific
```

### 3.Intel CPU Counters (msr)
Required for Intel PCM (CPU monitoring) to access model-specific registers.
```bash
sudo modprobe msr
```


## 🚀 Installation & Build

### 1️⃣ Clone repository (include submodules)

```bash
git clone https://github.com/X1ngChui/HwGauge.git --recursive
cd HwGauge/
```

### 2️⃣ Intel PCM Patch (optional but recommended)

Due to a submodule conflict, choose one of the following:

**Option A — Patch PCM tests (recommended)**
Edit `vendors/pcm/CMakeLists.txt` and comment/remove:

```
add_subdirectory(tests)
```

**Option B — Disable PCM entirely**

```bash
-DHWGAUGE_USE_INTEL_PCM=OFF
```

---

### 3️⃣ Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

---

## ⚙️ CMake Configuration Options

| Option                  | Default | Description                  |
| ----------------------- | ------- | ---------------------------- |
| `HWGAUGE_USE_INTEL_PCM` | `OFF`    | Enable Intel CPU collectors  |
| `HWGAUGE_USE_NVML`      | `OFF`    | Enable NVIDIA GPU collectors |
| `HWGAUGE_USE_NPU`       | `OFF`    | Enable Ascend NPU collectors |
| `HWGAUGE_USE_PROMETHEUS` | `OFF`  | Enable Prometheus exporte|
|`HWGAUGE_USE_POSTGRESQL`|`OFF`|Enable PostgreSQL storage|

Disable collectors you don't need to reduce dependencies.


---

## ▶️ Usage

After building, the binary is available in `bin/`.

### Run exporter (sudo required)

```bash
chmod +x bin/hwgauge
sudo ./bin/hwgauge [OPTIONS]
```

### Command-line options
```bash
sudo ./bin/hwgauge --help
```

---

## 📊 Exported Prometheus Metrics

### 🖥️ CPU (Intel PCM)

| Metric                        | Unit | Description              |
| ----------------------------- | ---- | ------------------------ |
| `cpu_utilization_percent`     | %    | CPU utilization          |
| `cpu_frequency_mhz`           | MHz  | Current frequency        |
| `cpu_c0_residency_percent`    | %    | Active state residency   |
| `cpu_c6_residency_percent`    | %    | Deep sleep residency     |
| `cpu_power_usage_watts`       | W    | CPU power draw           |
| `memory_read_bandwidth_mbps`  | MB/s | Memory read throughput   |
| `memory_write_bandwidth_mbps` | MB/s | Memory write throughput  |
| `memory_power_usage_watts`    | W    | Memory power consumption |

---

### 🎮 GPU (NVIDIA NVML)

| Metric                           | Unit | Description      |
| -------------------------------- | ---- | ---------------- |
| `gpu_utilization_percent`        | %    | Core utilization |
| `gpu_memory_utilization_percent` | %    | VRAM utilization |
| `gpu_frequency_mhz`              | MHz  | Core clock       |
| `gpu_memory_frequency_mhz`       | MHz  | Memory clock     |
| `gpu_power_usage_watts`          | W    | Power draw       |

---

### 🧠 NPU（Ascend DCMI）

| Metric                                 | Unit  | Description              |
| -------------------------------------- | ----- | ------------------------ |
| `npu_aicore_utilization_percent`       | %     | NPU AICore 利用率        |
| `npu_aicpu_utilization_percent`        | %     | NPU AICPU 利用率         |
| `npu_memory_utilization_percent`       | %     | NPU 内存利用率           |
| `npu_aicore_frequency_mhz`             | MHz   | NPU AICore 频率          |
| `npu_aicpu_frequency_mhz`              | MHz   | NPU AICPU 频率           |
| `npu_mem_frequency_mhz`                | MHz   | NPU 内存频率             |
| `npu_power_watts`                      | W     | NPU 功耗                 |
| `npu_health`                           | -     | NPU 健康状态             |
| `npu_temperature_celsius`              | °C    | NPU 温度                 |
| `npu_voltage_volts`                    | V     | NPU 电压                 |

---
### 📊 System (General)

| Metric                              | Unit | Description                         |
|-------------------------------------|------|-------------------------------------|
| `system_memory_total_bytes`           | B    | Total physical memory               |
| `system_memory_used_bytes`            | B    | Used physical memory                |
| `system_memory_utilization_percent`   | %    | Memory utilization                  |
| `system_disk_read_bytes_per_sec`      | B/s  | Total disk read rate                |
| `system_disk_write_bytes_per_sec`     | B/s  | Total disk write rate               |
| `system_disk_max_utilization_percent` | %    | Utilization of busiest disk         |
| `system_net_download_bytes_per_sec`   | B/s  | Total network download rate         |
| `system_net_upload_bytes_per_sec`     | B/s  | Total network upload rate           |
| `system_power_usage_watts`            | W    | Total system power                  |


