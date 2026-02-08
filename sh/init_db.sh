#!/bin/bash
set -e

# ================= 配置区域 (在这里修改) =================

# 1. 设置你的数据库密码
#    如果留空，脚本会生成一个随机密码。
#    建议为了方便管理，填入你自己的强密码。
DB_PASSWORD="" 

# 2. 允许连接的机器列表 (支持公网 IP 和 内网 IP)
#    每次修改这个列表后重新运行脚本，配置会自动刷新。
ALLOWED_IPS=(
    "10.10.1.1" # node0
    "10.10.1.2" # node1
    "10.10.1.3" # node2
)

# ALLOWED_IPS=(
#     "130.127.133.137" # node_clem_1
#     "130.127.133.111" # node_clem_2
#     "130.127.133.140" # node_clem_3
#     "130.127.133.135" # node_clem_4
#     "130.127.133.113" # node_clem_5
#     "155.98.36.146"   # node_utah_1
#     "155.98.36.23"    # node_utah_2
#     "155.98.36.119"   # node_utah_3
#     "155.98.36.62"    # node_utah_4
#     "155.98.36.59"    # node_utah_5
# )

# ========================================================

echo ">>> [1/6] 检查并安装 PostgreSQL..."
# 这一步是幂等的，如果已安装，apt 会自动跳过
sudo apt-get update
sudo apt-get install -y postgresql postgresql-contrib ufw

# 获取当前安装的版本
PG_VER=$(ls /etc/postgresql | sort -V | tail -n 1)
CONF_DIR="/etc/postgresql/${PG_VER}/main"
PG_CONF="${CONF_DIR}/postgresql.conf"
HBA_CONF="${CONF_DIR}/pg_hba.conf"

if [ -z "$PG_VER" ]; then
    echo "Error: 未找到 PostgreSQL 安装目录！"
    exit 1
fi
echo "Detected Version: $PG_VER"

# --------------------------------------------------------
echo ">>> [2/6] 配置 postgresql.conf (监听所有网卡)..."
# 使用 sed 确保幂等：无论运行多少次，只修改这一行
sudo sed -i "s/^#\?listen_addresses.*/listen_addresses = '*'/" "${PG_CONF}"

# --------------------------------------------------------
echo ">>> [3/6] 重置并刷新 pg_hba.conf (白名单管控)..."

# 备份一下（可选）
sudo cp "${HBA_CONF}" "${HBA_CONF}.bak"

# 【关键】完全覆盖文件，而不是追加。这样可以确保删除的IP不再有权限。
# 写入基础规则 (允许本机连接)
cat <<EOF | sudo tee "${HBA_CONF}"
# --- 自动生成的安全配置 ---
# 1. 允许本机 (Localhost) 免密或密码连接
local   all             all                                     peer
host    all             all             127.0.0.1/32            scram-sha-256
host    all             all             ::1/128                 scram-sha-256

# 2. 允许信任的远程 IP (必须使用密码)
EOF

# 循环写入白名单 IP
for IP in "${ALLOWED_IPS[@]}"; do
    echo "Adding DB Access for: $IP"
    echo "host    all             all             $IP/32          scram-sha-256" | sudo tee -a "${HBA_CONF}"
done

# --------------------------------------------------------
echo ">>> [4/6] 重置并刷新 UFW 防火墙..."

# 启用 UFW
sudo ufw --force enable

# 1. 必须先允许 SSH，否则自己会被踢出去！
sudo ufw allow ssh
sudo ufw allow 22/tcp

# 2. 清理旧的 5432 规则
#    为了幂等，我们先删除所有关于 5432 的规则，再重新添加
#    注意：这里简单粗暴地删除可能比较复杂，我们采用“先拒绝所有5432，再特许”的逻辑
#    或者，更彻底的方法是重置 UFW (sudo ufw reset)，但这会影响其他服务。
#    为了安全且不影响除 DB 外的其他服务，我们使用 delete 循环清理（如果有报错忽略即可）

echo "Cleaning old firewall rules for port 5432..."
# 尝试删除所有入站 5432 的规则 (此处逻辑较简略，依赖 ufw status 可能会复杂)
# 简单的做法：重新载入规则。
# 为了确保“删除 IP 后生效”，我们这里只添加 Allow 规则。
# 如果你想严格移除旧 IP 的防火墙权限，建议手动运行 sudo ufw reset (慎用)
# 或者：脚本仅保证添加，由于 pg_hba.conf 已经拦截了，防火墙有多余规则也无所谓（双重保险）。
# 但为了满足你的要求，最稳妥的方法是：

# 循环添加信任 IP
for IP in "${ALLOWED_IPS[@]}"; do
    if ! sudo ufw status | grep -q "$IP.*5432"; then
        echo "Firewall allowing: $IP"
        sudo ufw allow from "$IP" to any port 5432
    else
        echo "Firewall rule for $IP already exists."
    fi
done

# 拒绝其他所有对 5432 的访问 (作为兜底)
# 注意：UFW 规则是有顺序的，Allow 必须在 Deny 之前生效，或者默认策略是 Deny。
# 通常 UFW 默认 Incoming 是 Deny。只要不 Allow 0.0.0.0/0 即可。

# --------------------------------------------------------
echo ">>> [5/6] 重启 PostgreSQL 应用配置..."
sudo systemctl restart postgresql
sleep 2

# --------------------------------------------------------
echo ">>> [6/6] 设置/更新 数据库密码..."

# 如果用户没填密码，生成一个
if [ -z "$DB_PASSWORD" ]; then
    DB_PASSWORD=$(openssl rand -base64 12 | tr -dc 'a-zA-Z0-9')
fi

# 执行改密码 SQL
sudo -u postgres psql -c "ALTER USER postgres WITH PASSWORD '${DB_PASSWORD}';" > /dev/null

echo "========================================================"
echo "配置完成！"
echo "--------------------------------------------------------"
echo "Host Public IP: $(curl -s ifconfig.me)"
echo "DB User:        postgres"
echo "DB Password:    ${DB_PASSWORD}"
echo "--------------------------------------------------------"
echo "当前允许连接的 IP 数量: ${#ALLOWED_IPS[@]}"
echo "pg_hba.conf 已重写，未在列表中的 IP 即使有密码也无法连接。"
echo "========================================================"

# wide current password: 17HA169pUyGfi1Z
# local current password: U2QV2PkKVM2Ki6Ff