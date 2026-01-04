# HwGauge

**HwGauge** is a lightweight hardware power consumption exporter designed to provide Prometheus metrics for CPU and GPU energy usage. It leverages modern C++ to provide high-performance monitoring with minimal overhead.

## Features

* **CPU Monitoring**: Integration with **Intel PCM** (Processor Counter Monitor) for detailed Intel CPU metrics.
* **GPU Monitoring**: Integration with **NVIDIA NVML** for NVIDIA GPU power tracking.
* **Prometheus Exposer**: Built-in HTTP server to expose metrics at a configurable endpoint.

## Prerequisites

* **CMake**: Version 3.25 or higher is required.
* **Compiler**: A C++17 compatible compiler (GCC, Clang, or MSVC).
* **CUDA Toolkit**: Must be installed to enable NVIDIA GPU monitoring (NVML).
* **Permissions**: Root or sudo privileges are required to access hardware registers (especially for Intel PCM).

## Installation & Building

### 1. Clone the Repository

Ensure you use the `--recursive` flag to fetch all necessary submodules (spdlog, CLI11, prometheus-cpp, and pcm):

```bash
git clone https://github.com/X1ngChui/HwGauge.git --recursive
cd HwGauge/
```

### 2. Manual Fix for Intel PCM
Due to a library version conflict in the submodule, you have two choices regarding Intel PCM:
* Option A (Patching): Open vendors/pcm/CMakeLists.txt and remove or comment out the line `add_subdirectory(tests)`.
* Option B (Disabling): If you do not need Intel CPU metrics, you can simply disable the component during the CMake configuration step using -DHWGAUGE_USE_INTEL_PCM=OFF.

### 3. Build the Project

```bash
mkdir build && cd build/
cmake ..
cmake --build . --parallel
```

## Usage

After a successful build, the binary will be located in the `bin/` directory.

### Running the Exposer

You must run the application with `sudo` to allow the collectors to access hardware data:

```bash
chmod +x bin/hwgauge 
sudo bin/hwgauge [OPTIONS]
```

### Command Line Arguments

The application supports the following options:

| Option | Description | Default |
| --- | --- | --- |
| `-a, --address` | The IP address and port to start the Prometheus exposer. | `127.0.0.1:8000` |
| `-i, --interval` | The data collection interval in seconds. | `1` |

**Example:**

```bash
sudo bin/hwgauge --address 0.0.0.0:8080 --interval 2
```

## Configuration Options

You can toggle specific collectors during the CMake configuration phase:

* 
`HWGAUGE_USE_INTEL_PCM`: Enable/Disable Intel CPU collectors (Default: `ON`).


* 
`HWGAUGE_USE_NVML`: Enable/Disable Nvidia GPU collectors (Default: `ON`).
