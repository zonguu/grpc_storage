#include "test_utils.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "kvstore.grpc.pb.h"

using grpc::ClientContext;
using grpc::Status;

// 由 CMake 传入 server 可执行文件的绝对路径
#ifndef KVSTORE_SERVER_PATH
#define KVSTORE_SERVER_PATH "./kvstore_server"
#endif

void ServerFixture::SetUp() {
    // 创建临时目录
    char tmpl[] = "/tmp/kvstore_ut_XXXXXX";
    char* dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr) << "mkdtemp failed";
    temp_dir_ = dir;
    data_file_ = temp_dir_ / "test.dat";

    StartServer();
    CreateStub();
}

void ServerFixture::TearDown() {
    if (server_pid_ > 0) {
        KillServer(SIGTERM);
        // 等待最多 3 秒优雅退出
        int status = 0;
        pid_t ret = waitpid(server_pid_, &status, 0);
        if (ret <= 0) {
            // 超时未退出，强制杀死
            KillServer(SIGKILL);
            waitpid(server_pid_, &status, 0);
        }
        server_pid_ = -1;
    }
    // 清理临时目录
    if (!temp_dir_.empty()) {
        std::filesystem::remove_all(temp_dir_);
    }
}

int ServerFixture::GetFreePort() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return 0;
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return 0;
    }

    socklen_t len = sizeof(addr);
    if (getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        close(fd);
        return 0;
    }

    int port = ntohs(addr.sin_port);
    close(fd);
    return port;
}

void ServerFixture::StartServer() {
    server_port_ = GetFreePort();
    ASSERT_GT(server_port_, 0) << "Failed to get free port";

    pid_t pid = fork();
    ASSERT_GE(pid, 0) << "fork failed";

    if (pid == 0) {
        // 子进程：启动 server
        std::string addr = "0.0.0.0:" + std::to_string(server_port_);
        std::string file = data_file_.string();
        execl(KVSTORE_SERVER_PATH, "kvstore_server",
              addr.c_str(), file.c_str(), nullptr);
        // execl 失败
        std::cerr << "Failed to exec server: " << KVSTORE_SERVER_PATH << std::endl;
        _exit(1);
    }

    // 父进程
    server_pid_ = pid;
    // 等待 server 启动完成（简单轮询连接）
    std::string target = "127.0.0.1:" + std::to_string(server_port_);
    bool connected = false;
    for (int i = 0; i < 50; ++i) {
        usleep(10000);  // 10ms
        auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        if (channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::milliseconds(100))) {
            connected = true;
            break;
        }
    }
    ASSERT_TRUE(connected) << "Server failed to start on port " << server_port_;
}

void ServerFixture::CreateStub() {
    std::string target = "127.0.0.1:" + std::to_string(server_port_);
    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = kvstore::KVStore::NewStub(channel);
}

void ServerFixture::RestartServer() {
    TearDown();
    temp_dir_ = "/tmp/kvstore_ut_restarted_XXXXXX";
    char tmpl[] = "/tmp/kvstore_ut_restarted_XXXXXX";
    char* dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    temp_dir_ = dir;
    data_file_ = temp_dir_ / "test.dat";

    // 把旧数据文件复制过来（如果存在的话，模拟持久化重启）
    std::filesystem::path old_file = temp_dir_.parent_path() / "test.dat";
    // 注意：这里 RestartServer 的实现需要配合具体测试场景调整
    // 简单起见，重新创建空的
    StartServer();
    CreateStub();
}

void ServerFixture::KillServer(int sig) {
    if (server_pid_ > 0) {
        kill(server_pid_, sig);
    }
}

bool ServerFixture::Put(const std::string& key, const std::string& value) {
    kvstore::PutRequest req;
    req.set_key(key);
    req.set_value(value);
    kvstore::PutResponse resp;
    ClientContext ctx;
    Status s = stub_->Put(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "Put RPC failed: " << s.error_message();
        return false;
    }
    return resp.success();
}

bool ServerFixture::Get(const std::string& key, std::string& value) {
    kvstore::GetRequest req;
    req.set_key(key);
    kvstore::GetResponse resp;
    ClientContext ctx;
    Status s = stub_->Get(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "Get RPC failed: " << s.error_message();
        return false;
    }
    if (resp.found()) {
        value = resp.value();
        return true;
    }
    return false;
}

bool ServerFixture::Delete(const std::string& key) {
    kvstore::DeleteRequest req;
    req.set_key(key);
    kvstore::DeleteResponse resp;
    ClientContext ctx;
    Status s = stub_->Delete(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "Delete RPC failed: " << s.error_message();
        return false;
    }
    return resp.success();
}

bool ServerFixture::Exists(const std::string& key) {
    kvstore::ExistsRequest req;
    req.set_key(key);
    kvstore::ExistsResponse resp;
    ClientContext ctx;
    Status s = stub_->Exists(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "Exists RPC failed: " << s.error_message();
        return false;
    }
    return resp.exists();
}

size_t ServerFixture::BatchPut(
    const std::vector<std::pair<std::string, std::string>>& kvs) {
    kvstore::BatchPutRequest req;
    for (const auto& kv : kvs) {
        auto* p = req.add_pairs();
        p->set_key(kv.first);
        p->set_value(kv.second);
    }
    kvstore::BatchPutResponse resp;
    ClientContext ctx;
    Status s = stub_->BatchPut(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "BatchPut RPC failed: " << s.error_message();
        return 0;
    }
    return resp.succeeded_count();
}

std::vector<std::pair<std::string, std::string>> ServerFixture::BatchGet(
    const std::vector<std::string>& keys) {
    kvstore::BatchGetRequest req;
    for (const auto& k : keys) {
        req.add_keys(k);
    }
    kvstore::BatchGetResponse resp;
    ClientContext ctx;
    Status s = stub_->BatchGet(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "BatchGet RPC failed: " << s.error_message();
        return {};
    }
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& p : resp.pairs()) {
        result.emplace_back(p.key(), p.value());
    }
    return result;
}

size_t ServerFixture::BatchDelete(const std::vector<std::string>& keys) {
    kvstore::BatchDeleteRequest req;
    for (const auto& k : keys) {
        req.add_keys(k);
    }
    kvstore::BatchDeleteResponse resp;
    ClientContext ctx;
    Status s = stub_->BatchDelete(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "BatchDelete RPC failed: " << s.error_message();
        return 0;
    }
    return resp.deleted_count();
}

bool ServerFixture::CompareAndSwap(const std::string& key,
                                   const std::string& expected,
                                   const std::string& new_value,
                                   std::string* actual_value) {
    kvstore::CASRequest req;
    req.set_key(key);
    req.set_expected_value(expected);
    req.set_new_value(new_value);
    kvstore::CASResponse resp;
    ClientContext ctx;
    Status s = stub_->CompareAndSwap(&ctx, req, &resp);
    if (!s.ok()) {
        ADD_FAILURE() << "CAS RPC failed: " << s.error_message();
        return false;
    }
    if (!resp.success() && actual_value != nullptr && resp.key_existed()) {
        *actual_value = resp.actual_value();
    }
    return resp.success();
}
