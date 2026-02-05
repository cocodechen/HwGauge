#!/bin/bash
set -e

# 安装 Redis
sudo apt update
sudo apt install -y redis-server

REDIS_CONF="/etc/redis/redis.conf"

# 允许外部访问
sudo sed -i 's/^bind .*/bind 0.0.0.0/' $REDIS_CONF

# 关闭保护模式（内网环境才建议）
sudo sed -i 's/^protected-mode .*/protected-mode no/' $REDIS_CONF

# 启动并设置开机自启
sudo systemctl enable redis-server
sudo systemctl restart redis-server

# 放行端口（如果 ufw 存在）
if command -v ufw >/dev/null 2>&1; then
    sudo ufw allow 6379/tcp
fi

# 简单验证
redis-cli ping
