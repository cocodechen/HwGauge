# HwGauge

HwGauge is a lightweight hardware power-consumption exporter that exposes **CPU and GPU energy metrics as Prometheus Gauges**.
It is implemented in modern C++ to provide **high-performance monitoring with minimal overhead**.

## ✨ Features

* 🖥️ **CPU Monitoring** — Intel PCM (Processor Counter Monitor)
* 🎮 **GPU Monitoring** — NVIDIA NVML (CUDA Toolkit required)
* 📡 **Prometheus Exposer** — Built-in HTTP server with configurable endpoint
* ⚙️ **Template-based Collector Framework** — clean separation of metrics & hardware backends

---

## 📦 Prerequisites

| Requirement     | Notes                                     |
| --------------- | ----------------------------------------- |
| CMake ≥ 3.25    | Required for building                     |
| C++17 compiler  | GCC / Clang / MSVC                        |
| CUDA Toolkit    | Required for NVML GPU monitoring          |
| Root privileges | Needed to access hardware registers (PCM) |

---

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
| `HWGAUGE_USE_INTEL_PCM` | `ON`    | Enable Intel CPU collectors  |
| `HWGAUGE_USE_NVML`      | `ON`    | Enable NVIDIA GPU collectors |

Disable collectors you don't need to reduce dependencies.

---

## ▶️ Usage

After building, the binary is available in `bin/`.

### Run exporter (sudo required)

```bash
chmod +x bin/hwgauge
sudo bin/hwgauge [OPTIONS]
```

### Command-line options

| Option           | Description                 | Default          |
| ---------------- | --------------------------- | ---------------- |
| `-a, --address`  | HTTP exposer bind address   | `127.0.0.1:8000` |
| `-i, --interval` | Sampling interval (seconds) | `1`              |

**Example**

```bash
sudo bin/hwgauge --address 0.0.0.0:8080 --interval 2
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

## 🧩 Collector Architecture

HwGauge uses a **template-based strategy pattern**:

1. **Collector Wrapper (`CPUCollector<T>`, `GPUCollector<T>`)**

   * Registers Prometheus `Family` / `Gauge`
   * Manages update loop

2. **Hardware Implementation (`T impl`)**

   * Fetches real hardware metrics
   * PCM, NVML, or user-defined backend

No inheritance required — duck typing via templates.

---

## 🛠️ Extend — Add Your Own Collector

To support new hardware (AMD GPU, storage, etc.):

### 1️⃣ Define metric + label structs

Used for raw data & device identifiers.

### 2️⃣ Implement class `T` with methods

* `std::string name()`
* `std::vector<LabelStruct> labels()`
* `std::vector<MetricStruct> sample()`

(`sample()` size must match `labels()`)

### 3️⃣ Implement wrapper collector

* inherit `Collector`
* register Prometheus families
* update gauges in `collect()`

### 4️⃣ Register in `main.cpp`

```cpp
#ifdef HWGAUGE_USE_MYIMPL
exposer->add_collector<
    hwgauge::MyNewCollector<hwgauge::MyImpl>
>(hwgauge::MyImpl());
#endif
```
