#!/bin/bash
PS4='(run_test)$ '
set -x

# 默认需要测试的目录
EXEC_DIR_PARAM="${1:-../3-simple-fat16}"

# cd correct directory
cd "$(dirname "$0")"

# 设置需要测试的目录
EXEC_DIR=$(realpath "$EXEC_DIR_PARAM")

echo "正在测试: $EXEC_DIR"

./build_image.sh fat16.img

# build, mount simple_fat16 and test
fusermount -zu ./fat16
rm -rf ./fat16
mkdir -p ./fat16
make -C "$EXEC_DIR" clean
make -C "$EXEC_DIR" debug

# 确认 simple_fat16 可执行文件存在
if [ ! -f "$EXEC_DIR/simple_fat16" ]; then
    echo "编译可能失败。$EXEC_DIR/simple_fat16 不存在"
    exit 1
fi

"$EXEC_DIR"/simple_fat16 -s ./fat16 --img="./fat16.img"