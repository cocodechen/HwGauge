#!/bin/bash

# 遇到错误立即停止
set -e

echo ">>> 1. Installing PostgreSQL..."
sudo apt-get update
# 安装 PG 和常用工具
sudo apt-get install -y postgresql postgresql-contrib

# 自动获取 PostgreSQL 主版本号 (例如 14, 16)
# 取数字最大的那个版本作为主版本
PG_VER=$(ls /etc/postgresql | sort -V | tail -n 1)

if [ -z "$PG_VER" ]; then
    echo "Error: Could not detect PostgreSQL version."
    exit 1
fi

CONF_DIR="/etc/postgresql/${PG_VER}/main"
PG_CONF="${CONF_DIR}/postgresql.conf"
HBA_CONF="${CONF_DIR}/pg_hba.conf"

echo "Detected PostgreSQL version: ${PG_VER}"
echo "Config dir: ${CONF_DIR}"

# 2. 修改 postgresql.conf：监听所有地址
echo ">>> 2. Configuring listen_addresses..."
# 使用 sed 替换，确保无论运行多少次，只修改那一行，保持幂等
sudo sed -i "s/^#\?listen_addresses.*/listen_addresses = '*'/" "${PG_CONF}"

# 3. 修改 pg_hba.conf：允许所有网段访问（必须带密码！）
echo ">>> 3. Configuring pg_hba.conf (Allow 0.0.0.0/0 with PASSWORD)..."

# 定义配置内容：使用 scram-sha-256 (密码验证)，而不是 trust (不安全)
# 这一步非常关键，之前的 trust 导致了被注入
NEW_RULE="host    all             all             0.0.0.0/0               scram-sha-256"
NEW_RULE_IPV6="host    all             all             ::/0                    scram-sha-256"

# 检查是否已经存在，不存在才追加（幂等性）
if grep -qF "0.0.0.0/0" "${HBA_CONF}"; then
    echo "Configuration for 0.0.0.0/0 already exists, skipping append."
else
    echo "Appending new rules to pg_hba.conf..."
    # 这里的配置意思：允许任何IP连接，但必须提供正确的密码
    echo "$NEW_RULE" | sudo tee -a "${HBA_CONF}"
    echo "$NEW_RULE_IPV6" | sudo tee -a "${HBA_CONF}"
fi

# 4. 重启 PostgreSQL 应用配置
echo ">>> 4. Restarting PostgreSQL service..."
sudo systemctl restart postgresql

# 等待几秒确保服务完全启动，防止立即执行 psql 报错
sleep 3

# 检查服务状态
if systemctl is-active --quiet postgresql; then
    echo "PostgreSQL is running."
else
    echo "Error: PostgreSQL failed to start. Check logs with: journalctl -u postgresql"
    exit 1
fi

# 5. 修改 postgres 用户密码
echo ">>> 5. Setting 'postgres' user password to '123456'..."
# 使用 sudo -u postgres 切换用户执行 SQL 命令
# 这里的 123456 是你要求的密码
if sudo -u postgres psql -c "ALTER USER postgres WITH PASSWORD '123456';" > /dev/null; then
    echo "Password set successfully."
else
    echo "Error: Failed to set password."
    exit 1
fi

echo "========================================================"
echo "SUCCESS! PostgreSQL ${PG_VER} is installed and configured."
echo "Connection Info:"
echo "  - Host: (Your Server Public IP)"
echo "  - Port: 5432"
echo "  - User: postgres"
echo "  - Pass: 123456"
echo "========================================================"