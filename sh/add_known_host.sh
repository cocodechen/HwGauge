#!/bin/bash

# 从 hosts.ini 提取所有 IP 地址并添加到 known_hosts
INVENTORY="../cluster/hosts_wide.ini"
grep "ansible_host=" $INVENTORY | awk -F'=' '{print $2}' | while read ip; do
    echo "Adding host key for $ip ..."
    ssh-keyscan -H $ip >> ~/.ssh/known_hosts 2>/dev/null
done
echo "All host keys added successfully!"

#!/bin/bash

# 定义文件路径
# SSH_CMD_FILE="../cluster/ssh.txt"
# KNOWN_HOSTS_FILE=~/.ssh/known_hosts

# echo "正在从 $SSH_CMD_FILE 读取并添加主机指纹..."

# # 同样加上 || [ -n "$line" ] 防止漏掉最后一行
# while read -r line || [ -n "$line" ]; do
#     # 跳过空行
#     [[ -z "$line" ]] && continue

#     # 1. 提取 hostname
#     # 逻辑：取 ssh 命令的第2列 (xc@hostname)，再用 @ 切割取后面部分
#     HOSTNAME=$(echo "$line" | awk '{print $2}' | cut -d'@' -f2)

#     echo "Adding host key for $HOSTNAME ..."
    
#     # 2. 直接对 hostname 进行 keyscan，不再转为 IP
#     ssh-keyscan -H "$HOSTNAME" >> "$KNOWN_HOSTS_FILE" 2>/dev/null

# done < "$SSH_CMD_FILE"

# echo "All host keys added successfully!"