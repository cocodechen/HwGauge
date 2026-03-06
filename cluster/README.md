# 使用说明

## Cluster 文件说明

### 基础配置文件
| 文件名 | 功能描述 |
| :--- | :--- |
| **ssh.txt** | 记录各个节点的 SSH 连接命令 |
| **ansible.cfg** | Ansible 配置文件，需在其中指定使用的 inventory（主机清单）文件 |

### 主机清单 (Inventory)
| 文件名 | 功能描述 |
| :--- | :--- |
| **hosts_local.ini** | 适用于局域网多节点环境 |
| **hosts_wide.ini** | 适用于公网多节点环境 |

### Ansible Playbooks (任务脚本)
| 文件名 | 功能描述 |
| :--- | :--- |
| **run_sh.yml** | **批量运行脚本**：在多节点上执行指定的 Shell 脚本任务。 |
| **run_sh_reboot.yml** | **运行并重启**：在多节点运行指定脚本后，执行系统重启。 |
| **start.yml** | **启动负载任务**：多节点启动负载生成及数据采集任务。 |
| **stop.yml** | **停止负载任务**：停止由 `start.yml` 启动的服务。 |
| **start_test.yml** | **启动采集任务**：仅启动多节点数据采集任务。 |
| **stop_test.yml** | **停止采集任务**：停止由 `start_test.yml` 启动的服务。 |

---

## 多节点使用

### 1.节点准备
1.选取一台服务器作为**主节点**，并在网页设置其 SSH Key。
2.收集其他节点的 SSH 连接命令，填写到 `ssh.txt` 文件中。

### 2.配置 IP 与信任
3.  运行脚本自动获取 IP 地址：
```bash
./get_ip.sh
```
4.  将获取到的 IP 填入对应的 `hosts.ini` 文件中。
5.  添加节点到 SSH 信任列表：
```bash
./add_known_hosts
```

### 主节点运行
6.在主节点上执行环境与服务初始化：
```bash
# 初始化环境
./set_env.sh

# 初始化数据库（注意管理权限与密码）
./init_db.sh

# 初始化 Redis
./init_redis.sh
```

### 其他节点运行
7.在主节点使用Ansible批量操作其他节点：
```bash
#初始化环境：注意运行前请修改 run_sh.yml 中指定的脚本名称及参数
ansible-playbook run_sh.yml

#启动任务：注意运行前请修改 start.yml 中的参数（包括特定服务的密码等）
ansible-playbook start.yml

#停止任务
ansible-playbook stop.yml
```