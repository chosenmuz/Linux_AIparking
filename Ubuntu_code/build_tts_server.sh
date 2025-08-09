#!/bin/bash

# 创建编译目录
mkdir -p build

# 检测系统架构
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    LIB_DIR="libs/x64"
else
    LIB_DIR="libs/x86"
fi

echo "检测到系统架构: $ARCH"
echo "使用库文件目录: $LIB_DIR"

# 编译TTS服务程序
gcc -o build/tts_server \
    samples/xtts_offline_sample/xtts_offline_sample.c \
    -I include \
    -L $LIB_DIR \
    -lmsc \
    -lpthread \
    -ldl \
    -Wl,-rpath,'$ORIGIN/../'$LIB_DIR

if [ $? -eq 0 ]; then
    echo "编译成功！TTS服务程序已生成: build/tts_server"
    echo "使用方法："
    echo "1. 在PC上运行: ./build/tts_server"
    echo "2. 确保PC的IP地址为192.168.9.100，监听50001端口"
    echo "3. 在开发板上运行Audio模块，它会连接到PC进行语音合成"
else
    echo "编译失败！"
    exit 1
fi 