# 使用说明

## 脚本说明

| 脚本 | 作用 |
|---|---|
| `get_ip.sh` | 根据 `../cluster/ssh.txt` 获取各节点公网 IP |
| `add_known_hosts` | 从 `hosts_local/hosts_wide.ini` 提取所有 IP，并加入 `known_hosts` |
| `gpu_drive.sh` | 安装 GPU 驱动（需修改为对应版本） |
| `cuda_toolkit.sh` | 安装 CUDA Toolkit（需修改为对应版本） |
| `init_db.sh` | 初始化数据库服务 |
| `init_redis.sh` | 初始化 Redis 服务 |
| `set_env.sh` | 初始化当前运行环境 |

---

## 单节点使用

### 1. GPU 环境（如需要）

```bash
./gpu_drive.sh
./cuda_toolkit.sh
```

### 2. 初始化运行环境
```bash
./set_env.sh [-c/-g/-n]
```

### 3. 初始化数据库
注意加入节点的权限，并记住设置的密码
```bash
./init_db.sh
```

### 4. 初始化Redis服务（如需要）
```bash
./init_redis.sh
```
