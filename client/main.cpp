#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

class KVStoreClient {
public:
    KVStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    // 发送Put请求
    bool Put(const std::string& key, const std::string& value) {
        PutRequest request;
        request.set_key(key);
        request.set_value(value);

        PutResponse response;
        ClientContext context;

        Status status = stub_->Put(&context, request, &response);

        if (status.ok()) {
            std::cout << "Put " << (response.success() ? "succeeded" : "failed")
                      << ": " << response.message() << std::endl;
            return response.success();
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    // 发送Get请求
    bool Get(const std::string& key, std::string& value) {
        GetRequest request;
        request.set_key(key);

        GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);

        if (status.ok()) {
            if (response.found()) {
                value = response.value();
                std::cout << "Get succeeded: key=" << key << ", value=" << value << std::endl;
                return true;
            } else {
                std::cout << "Get: key not found (" << response.message() << ")" << std::endl;
                return false;
            }
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<KVStore::Stub> stub_;
};

void PrintUsage(const char* program) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << program << " put <key> <value>  - Store key-value pair" << std::endl;
    std::cout << "  " << program << " get <key>          - Retrieve value by key" << std::endl;
    std::cout << "  " << program << " <key> <value>      - Store key-value pair (shorthand)" << std::endl;
}

int main(int argc, char** argv) {
    std::string server_address = "localhost:50052";

    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    // 创建客户端
    KVStoreClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::string cmd = argv[1];

    if (cmd == "put" && argc == 4) {
        // put命令: client put <key> <value>
        std::string key = argv[2];
        std::string value = argv[3];
        client.Put(key, value);
    } else if (cmd == "get" && argc == 3) {
        // get命令: client get <key>
        std::string key = argv[2];
        std::string value;
        client.Get(key, value);
    } else if (argc == 3) {
        // 简写形式: client <key> <value>
        std::string key = argv[1];
        std::string value = argv[2];
        client.Put(key, value);
    } else {
        PrintUsage(argv[0]);
        return 1;
    }

    return 0;
}
