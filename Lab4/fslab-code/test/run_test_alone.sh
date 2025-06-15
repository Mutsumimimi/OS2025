#!/bin/bash
PS4='(run_test)$ '
set -x

# cd correct directory
cd "$(dirname "$0")"

# 确认 ./fat16 已经是挂载状态
if ! mountpoint -q ./fat16; then
    echo "./fat16 未被正确挂载"
    exit 1
fi

python3 -m pytest --capture=tee-sys -x -v ./fat16_test.py
fusermount -zu ./fat16
