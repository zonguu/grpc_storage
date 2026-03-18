# gRPC KV存储应用

一个基于gRPC的键值存储服务，服务端提供Put/Get接口，数据持久化到文件。

## 项目结构

```
.
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 本文件
├── proto/
│   └── kvstore.proto       # Protobuf服务定义
├── server/
│   ├── main.cpp            # 服务端入口
│   ├── kv_store.h          # KV存储引擎头文件
│   └── kv_store.cpp        # KV存储引擎实现
├── client/
│   └── main.cpp            # 客户端入口
└── build/                  # 构建输出目录
```

## 安装依赖

系统需要安装以下开发包：

```bash
sudo apt-get update
sudo apt-get install -y libgrpc++-dev protobuf-compiler-grpc
```

## 构建项目

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

## 使用方法

### 启动服务端

```bash
./kvstore_server [address] [data_file]

# 默认配置
./kvstore_server

# 自定义配置
./kvstore_server 0.0.0.0:50051 /tmp/kvstore.dat
```

### 使用客户端

```bash
# 写入键值对
./kvstore_client put mykey myvalue

# 或者简写形式
./kvstore_client mykey myvalue

# 读取键值对
./kvstore_client get mykey
```

## 示例

终端1 - 启动服务端：
```bash
$ ./kvstore_server
KVStore Server listening on 0.0.0.0:50051
Data file: kvstore.dat
```

终端2 - 客户端操作：
```bash
$ ./kvstore_client put name alice
Put succeeded: Put successful

$ ./kvstore_client get name
Get succeeded: key=name, value=alice

$ ./kvstore_client get nonexistent
Get: key not found (Key not found)
```

## 特性

- 基于gRPC的高性能RPC通信
- 支持Put/Get基本KV操作
- 数据持久化到文件
- 线程安全的存储引擎
- 支持并发访问
