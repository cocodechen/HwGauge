#!/bin/bash

# 数据库配置
DB_NAME="postgres"
DB_PASSWORD="WaflT3NHu3dooAt"

# 输出目录
BACKUP_DIR="../pg_backup"
mkdir -p "$BACKUP_DIR"

# 时间戳
TIME=$(date +"%Y%m%d_%H%M%S")

# 设置密码环境变量
export PGPASSWORD="$DB_PASSWORD"

# 判断是否压缩
if [ "$1" == "-c" ]; then
    OUT_FILE="$BACKUP_DIR/${DB_NAME}_dump_${TIME}.dump"
    
    sudo -u postgres pg_dump -d "$DB_NAME" -Fc > "$OUT_FILE"
else
    OUT_FILE="$BACKUP_DIR/${DB_NAME}_dump_${TIME}.sql"
    
    sudo -u postgres pg_dump -d "$DB_NAME" --column-inserts > "$OUT_FILE"
fi

# 检查结果
if [ $? -eq 0 ]; then
    echo "Backup successful: $OUT_FILE"
else
    echo "Backup failed"
fi