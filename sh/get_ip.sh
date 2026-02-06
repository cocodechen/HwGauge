#!/bin/bash

SSH_FILE="../cluster/ssh.txt"

# 修改点在这里：加上 || [ -n "$cmd" ] 以处理最后一行无换行的情况
while read -r cmd || [ -n "$cmd" ]; do
    # 跳过空行
    [[ -z "$cmd" ]] && continue
    
    # 提取 hostname (例如 pc762.emulab.net)
    HOSTNAME=$(echo "$cmd" | awk '{print $2}' | cut -d'@' -f2)
    
    # 解析 IP
    IP=$(dig +short "$HOSTNAME" | head -n 1)
    
    if [ ! -z "$IP" ]; then
        echo "$HOSTNAME 的 IP 是: $IP"
    else
        echo "$HOSTNAME 解析失败"
    fi
done < "$SSH_FILE"