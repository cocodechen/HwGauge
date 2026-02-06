#!/bin/bash

# ==============================================================================
# 脚本名称: set_env.sh
# 功能: 自动化安装 HwGauge 及其依赖环境
# 使用方法: ./set_env.sh [-c] [-g] [-n]
# 参数说明:
#   -c : 启用 CPU 采集 (Intel PCM)
#   -g : 启用 GPU 采集 (NVML)
#   -n : 启用 NPU 采集 (Ascend)
# ==============================================================================

set -e  # 遇到错误立即停止
set -x  # 开启调试模式，打印执行的每一行命令

source ~/.bashrc

# 1. 初始化变量 (默认为 OFF)
USE_CPU="OFF"
USE_GPU="OFF"
USE_NPU="OFF"

# 2. 解析命令行参数
while getopts "cgn" opt; do
  case $opt in
    c) USE_CPU="ON" ;;
    g) USE_GPU="ON" ;;
    n) USE_NPU="ON" ;;
    *) echo "用法: $0 [-c (CPU)] [-g (GPU)] [-n (NPU)]"; exit 1 ;;
  esac
done

echo "=== 配置信息: CPU=${USE_CPU}, GPU=${USE_GPU}, NPU=${USE_NPU} ==="

# 3. 安装基础工具和 PostgreSQL 依赖
echo "=== 正在更新 apt 并安装基础依赖 ==="
sudo apt-get update
# build-essential 包含 gcc/g++, make 等
sudo apt-get install -y build-essential git wget pkg-config
# PostgreSQL 依赖
sudo apt-get install -y postgresql-client libpq-dev
# Redis 依赖
sudo apt-get install -y libhiredis-dev
# Ansible 依赖
sudo apt update && sudo apt install -y ansible
# 硬件监控依赖
sudo apt-get install -y ipmitool lm-sensors linux-modules-extra-$(uname -r)

# ================= GPU 模式 =================
export PATH=/usr/local/cuda-12.2/bin:$PATH
if [ "$USE_GPU" == "ON" ]; then
    echo "=== [GPU模式] 检查 NVIDIA 驱动和 CUDA 环境 ==="

    # 1. 检查 NVIDIA 驱动（nvidia-smi 是唯一可信标志）
    if ! command -v nvidia-smi &> /dev/null; then
        echo "ERROR: 未检测到 NVIDIA 驱动（nvidia-smi 不存在）。"
        echo "请先正确安装 NVIDIA 驱动后再运行该脚本。"
        exit 1
    fi

    # 2. 检查 CUDA Toolkit（nvcc）
    if ! command -v nvcc &> /dev/null; then
        echo "ERROR: 未检测到 CUDA Toolkit（nvcc 不存在）。"
        echo "请先正确安装 CUDA Toolkit 后再运行该脚本。"
        exit 1
    fi

    echo "检测到 NVIDIA 驱动: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -n1)"
    echo "检测到 CUDA: $(nvcc --version | grep release)"
fi


# ================= NPU 模式 =================
if [ "$USE_NPU" == "ON" ]; then
    echo "=== [NPU模式] 检查 Ascend 环境 ==="

    if [ ! -d "/usr/local/Ascend" ] || [ ! -d "/usr/local/Ascend/driver" ]; then
        echo "ERROR: 未检测到 Ascend NPU 驱动环境。"
        echo "请确认 CloudLab 镜像已正确安装 Ascend Driver / CANN。"
        exit 1
    fi

    echo "检测到 Ascend 目录存在。"
fi

# 4. 安装 CMake 4.2.1 (如果未安装)
if ! command -v cmake &> /dev/null || [[ $(cmake --version | head -n1) != *"version 4."* ]]; then
    echo "=== 正在安装 CMake 4.2.1 ==="
    # 强行用 sudo 删除旧文件，不管它存不存在
    sudo rm -f /tmp/cmake-install.sh
    # 下载
    wget https://github.com/Kitware/CMake/releases/download/v4.2.1/cmake-4.2.1-linux-x86_64.sh -O /tmp/cmake-install.sh
    chmod +x /tmp/cmake-install.sh
    
    # 安装到 /opt/cmake
    sudo mkdir -p /opt/cmake
    sudo /tmp/cmake-install.sh --prefix=/opt/cmake --skip-license --exclude-subdir
    
    # 配置环境变量 (写入 .bashrc 以便未来使用)
    if ! grep -q "/opt/cmake/bin" ~/.bashrc; then
        echo 'export PATH=/opt/cmake/bin:$PATH' >> ~/.bashrc
    fi
    
    # 临时导出环境变量供本次脚本使用
    export PATH=/opt/cmake/bin:$PATH
else
    echo "=== CMake 已安装，跳过 ==="
fi
# 验证版本
cmake --version

# 5. 克隆代码
PROJECT_DIR="$HOME/HwGauge"
GITIGNORE="$PROJECT_DIR/.gitignore"
if [ -d "$PROJECT_DIR" ]; then
    echo "=== 项目目录已存在，执行 git pull 更新 ==="
    cd "$PROJECT_DIR"
    git pull
    git submodule update --init --recursive
else
    echo "=== 正在克隆 HwGauge 代码 ==="
    cd "$HOME"
    git clone https://github.com/cocodechen/HwGauge.git --recursive
    cd "$PROJECT_DIR"
fi
# ---- 确保 build 在 .gitignore 中 ----
touch "$GITIGNORE"

if ! grep -qx "build/" "$GITIGNORE"; then
    echo "build/" >> "$GITIGNORE"
    echo "=== 已将 build/ 加入 .gitignore ==="
fi

# 如果 build 之前被 git 跟踪，解除跟踪
if git ls-files --error-unmatch build >/dev/null 2>&1; then
    git rm -r --cached build
    echo "=== 已从 git 索引中移除 build 目录 ==="
fi

# 6. 打补丁 (Intel PCM CMakeLists.txt)
# 你的要求：给 vendors/pcm/CMakeLists.txt 第 186 行加上注释
echo "=== 正在修改 vendors/pcm/CMakeLists.txt ==="
PCM_CMAKE="$PROJECT_DIR/vendors/pcm/CMakeLists.txt"
if [ -f "$PCM_CMAKE" ]; then
    # 使用 sed 命令在第 186 行行首添加 #
    # 注意：为了防止多次运行导致变成 ##，可以先判断一下
    sed -i '186s/^[^#]/#&/' "$PCM_CMAKE"
    echo "已注释 $PCM_CMAKE 第 186 行"
else
    echo "警告: 未找到 $PCM_CMAKE，跳过补丁"
fi

# 7. 编译项目
echo "=== 开始编译 ==="
mkdir -p build && cd build
# 清理旧的编译文件
rm -rf *

# 执行 CMake
cmake -DHWGAUGE_USE_INTEL_PCM=${USE_CPU} \
      -DHWGAUGE_USE_NVML=${USE_GPU} \
      -DHWGAUGE_USE_NPU=${USE_NPU} \
      -DHWGAUGE_USE_CLUSTER=ON\
      -DHWGAUGE_USE_PROMETHEUS=ON \
      -DHWGAUGE_USE_POSTGRESQL=ON \
      ..

# 开始编译 (使用 -j 自动利用多核 CPU 加速)
make -j$(nproc)

echo "=== 编译完成，可执行文件位于 $PROJECT_DIR/build/bin/ ==="

# 8. 加载内核模块 (需要 sudo 权限)
echo "=== 加载内核模块 ==="
sudo modprobe msr || echo "警告: 加载 msr 失败 (可能需要在裸机环境)"
sudo modprobe coretemp || echo "警告: 加载 coretemp 失败"

# IPMI 模块
sudo modprobe ipmi_devintf || true
sudo modprobe ipmi_si || true

echo "=== 脚本执行完毕 ==="