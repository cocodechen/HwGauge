#!/bin/bash

set -e

# 自动获取 PostgreSQL 主版本号（如 12 / 14 / 16）
PG_VER=$(ls /etc/postgresql)
CONF_DIR="/etc/postgresql/${PG_VER}/main"

PG_CONF="${CONF_DIR}/postgresql.conf"
HBA_CONF="${CONF_DIR}/pg_hba.conf"

echo "Detected PostgreSQL version: ${PG_VER}"
echo "Config dir: ${CONF_DIR}"

# 1. 修改 postgresql.conf：监听所有地址
echo "Updating listen_addresses..."
sudo sed -i "s/^#\?listen_addresses.*/listen_addresses = '*'/" "${PG_CONF}"

# 2. 修改 pg_hba.conf：允许 10.10.1.0/24 trust 访问
echo "Updating pg_hba.conf..."
sudo tee -a "${HBA_CONF}" > /dev/null <<'EOF'

# Allow 10.10.1.0/24 without password (experiment only)
host    all             all             10.10.1.0/24            trust
EOF

# 3. 启动并启用 PostgreSQL
echo "Starting PostgreSQL..."
sudo systemctl start postgresql
sudo systemctl enable postgresql

# 4. 检查状态
echo "Checking status..."
if systemctl is-active --quiet postgresql; then
    echo "PostgreSQL is running."
    systemctl status postgresql --no-pager
else
    echo "Error: PostgreSQL failed to start."
    exit 1
fi

# 5. 重载配置
echo "Restarting PostgreSQL config..."
sudo systemctl restart postgresql

# 6. 修改 postgres 用户密码
echo "Setting postgres password..."
sudo -u postgres psql -c "ALTER USER postgres WITH PASSWORD '123456';"

echo "PostgreSQL configuration completed successfully."
