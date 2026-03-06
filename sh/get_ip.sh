#!/bin/bash
# 用法:
# ./get_ip.sh public
# ./get_ip.sh private

SSH_FILE="../cluster/ssh.txt"
MODE=$1

if [[ "$MODE" != "public" && "$MODE" != "private" ]]; then
    echo "用法: $0 [public|private]"
    exit 1
fi

# 如果是局域网，直接输出 /etc/hosts
if [[ "$MODE" == "private" ]]; then
    cat /etc/hosts
    exit 0
fi

# 公网逻辑保持原样
while read -r cmd || [ -n "$cmd" ]; do
    [[ -z "$cmd" ]] && continue

    HOSTNAME=$(echo "$cmd" | awk '{print $2}' | cut -d'@' -f2)

    IP=$(dig +short "$HOSTNAME" | head -n 1)

    if [ ! -z "$IP" ]; then
        echo "$HOSTNAME 的 IP 是: $IP"
    else
        echo "$HOSTNAME 解析失败"
    fi

done < "$SSH_FILE"