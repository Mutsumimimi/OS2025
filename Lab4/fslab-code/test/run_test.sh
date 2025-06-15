#!/bin/bash
PS4='(run_test)$ '
set -x

# 默认需要测试的目录
EXEC_DIR_PARAM="${1:-.}"

# 设置需要测试的目录
EXEC_DIR=$(realpath "$EXEC_DIR_PARAM")

echo "正在测试: $EXEC_DIR"

# cd correct directory
cd "$(dirname "$0")"

./run_mount_bg.sh "$EXEC_DIR"
./run_test_alone.sh
