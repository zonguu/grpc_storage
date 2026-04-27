#pragma once

#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <string>
#include <memory>
#include <filesystem>

#include "kvstore.grpc.pb.h"

// ServerFixture: 负责在独立进程中启动 kvstore_server，并提供 client stub
class ServerFixture : public ::testing::Test {
protected:
    // 每个测试用例的独立临时目录
    std::filesystem::path temp_dir_;
    // 数据文件路径
    std::filesystem::path data_file_;
    // server 监听的端口
    int server_port_ = 0;
    // server 进程 PID
    pid_t server_pid_ = -1;
    // client stub
    std::unique_ptr<kvstore::KVStore::Stub> stub_;

    void SetUp() override;
    void TearDown() override;

    // 向指定 key 写入 value
    bool Put(const std::string& key, const std::string& value);
    // 读取指定 key，返回是否找到
    bool Get(const std::string& key, std::string& value);
    // 删除指定 key
    bool Delete(const std::string& key);
    // 检查 key 是否存在
    bool Exists(const std::string& key);
    // 批量写入
    size_t BatchPut(const std::vector<std::pair<std::string, std::string>>& kvs);
    // 批量读取
    std::vector<std::pair<std::string, std::string>> BatchGet(
        const std::vector<std::string>& keys);
    // 批量删除
    size_t BatchDelete(const std::vector<std::string>& keys);
    // CAS
    bool CompareAndSwap(const std::string& key,
                        const std::string& expected,
                        const std::string& new_value,
                        std::string* actual_value = nullptr);

    // 优雅重启 server（SIGTERM 后重新启动）
    void RestartServer();
    // 向 server 发信号
    void KillServer(int sig);

private:
    // 启动 server 进程
    void StartServer();
    // 获取一个系统分配的空闲端口
    static int GetFreePort();
    // 创建 client stub
    void CreateStub();
};
