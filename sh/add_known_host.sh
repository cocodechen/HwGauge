#!/bin/bash

# 从 hosts.ini 提取所有 IP 地址并添加到 known_hosts
INVENTORY="../cluster/hosts.ini"
grep "ansible_host=" $INVENTORY | awk -F'=' '{print $2}' | while read ip; do
    echo "Adding host key for $ip ..."
    ssh-keyscan -H $ip >> ~/.ssh/known_hosts 2>/dev/null
done
echo "All host keys added successfully!"