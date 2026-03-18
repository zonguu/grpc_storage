#!/bin/bash
# CMake 构建脚本

set -e

BUILD_DIR=${BUILD_DIR:-build}
JOBS=${JOBS:-4}

echo "=== Building gRPC KV Store ==="

# 创建构建目录
mkdir -p $BUILD_DIR

# 运行 CMake
echo "[1/2] Configuring with CMake..."
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
echo ""
echo "[2/2] Building..."
make -j$JOBS

cd ..

echo ""
echo "=== Build Complete ==="
echo "  Server: ./$BUILD_DIR/kvstore_server"
echo "  Client: ./$BUILD_DIR/kvstore_client"
echo ""
echo "Usage:"
echo "  Terminal 1: ./$BUILD_DIR/kvstore_server"
echo "  Terminal 2: ./$BUILD_DIR/kvstore_client put mykey myvalue"
echo "              ./$BUILD_DIR/kvstore_client get mykey"
