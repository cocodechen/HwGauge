#!/bin/bash
set -e

# 1. 安装 Redis
echo "Installing Redis..."
sudo apt-get update
sudo apt-get install -y redis-server

REDIS_CONF="/etc/redis/redis.conf"
# 这里的密码改成你想要的，绝对不要留空！
REDIS_PASS="123456"

echo "Configuring Redis..."

# 2. 备份配置文件 (好习惯)
if [ ! -f "${REDIS_CONF}.bak" ]; then
    sudo cp "$REDIS_CONF" "${REDIS_CONF}.bak"
fi

# 3. 处理 bind 配置 (幂等操作)
# 先把所有以 bind 开头的行（包括被注释的）全部注释掉，防止冲突
# 这样 Redis 就会读取我们在文件末尾追加的配置
sudo sed -i 's/^bind/# bind/' "$REDIS_CONF"
# 同理处理 protected-mode
sudo sed -i 's/^protected-mode/# protected-mode/' "$REDIS_CONF"
# 同理处理 requirepass (如果原文件有的话)
sudo sed -i 's/^requirepass/# requirepass/' "$REDIS_CONF"

# 4. 追加自定义配置 (幂等检查)
# 我们定义一个标记，如果文件里包含这个标记，说明已经配置过了
MY_CONFIG_MARK="# --- CUSTOM CONFIG BY SCRIPT ---"

if grep -qF "$MY_CONFIG_MARK" "$REDIS_CONF"; then
    echo "Redis configuration already exists. Skipping append."
else
    echo "Appending new configuration..."
    # 使用 tee -a 追加配置到文件末尾
    # bind 0.0.0.0: 允许公网连接
    # protected-mode no: 允许非本地连接
    # requirepass: 设置密码 (必须!)
    sudo tee -a "$REDIS_CONF" > /dev/null <<EOF

$MY_CONFIG_MARK
bind 0.0.0.0
protected-mode no
requirepass $REDIS_PASS
EOF
fi

# 5. 重启 Redis
echo "Restarting Redis..."
sudo systemctl enable redis-server
sudo systemctl restart redis-server

# 6. 放行端口 (如果有 UFW)
if command -v ufw >/dev/null 2>&1; then
    echo "Configuring UFW..."
    sudo ufw allow 6379/tcp
fi

# 7. 验证
echo "Checking Redis status..."
# 等待服务启动
sleep 2

if systemctl is-active --quiet redis-server; then
    echo "Redis is running."
    echo "Testing connection with password..."
    # 尝试用密码 ping
    if redis-cli -a "$REDIS_PASS" ping | grep -q "PONG"; then
        echo "SUCCESS: Redis is online and password protected."
    else
        echo "WARNING: Redis is running but ping failed."
    fi
else
    echo "ERROR: Redis failed to start."
    exit 1
fi