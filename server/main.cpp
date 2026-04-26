#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"
#include "storage_engine.h"
#include "file_storage_engine.h"
#include "memory_storage_engine.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using kvstore::BatchDeleteRequest;
using kvstore::BatchDeleteResponse;
using kvstore::BatchGetRequest;
using kvstore::BatchGetResponse;
using kvstore::BatchPutRequest;
using kvstore::BatchPutResponse;
using kvstore::CASRequest;
using kvstore::CASResponse;
using kvstore::DeleteRequest;
using kvstore::DeleteResponse;
using kvstore::ExistsRequest;
using kvstore::ExistsResponse;
using kvstore::GetRequest;
using kvstore::GetResponse;
using kvstore::KVPair;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutResponse;

namespace kvstore {

// KV存储服务实现
class KVStoreServiceImpl final : public KVStore::Service {
public:
    // 构造函数接收存储引擎指针，可以是任何实现了 StorageEngine 接口的引擎
    explicit KVStoreServiceImpl(std::unique_ptr<StorageEngine> engine) 
        : storage_engine_(std::move(engine)) {}

    Status Put(ServerContext* context, 
               const PutRequest* request, 
               PutResponse* response) override {
        std::cout << "Put: key=" << request->key() << std::endl;
        
        bool success = storage_engine_->Put(request->key(), request->value());
        
        response->set_success(success);
        response->set_message(success ? "Put successful" : "Put failed");
        return Status::OK;
    }

    Status Get(ServerContext* context, 
               const GetRequest* request, 
               GetResponse* response) override {
        std::cout << "Get: key=" << request->key() << std::endl;
        
        std::string value;
        bool found = storage_engine_->Get(request->key(), value);
        
        response->set_found(found);
        if (found) {
            response->set_value(value);
            response->set_message("Key found");
        } else {
            response->set_message("Key not found");
        }
        return Status::OK;
    }

    Status Delete(ServerContext* context,
                  const DeleteRequest* request,
                  DeleteResponse* response) override {
        std::cout << "Delete: key=" << request->key() << std::endl;

        bool success = storage_engine_->Delete(request->key());

        response->set_success(success);
        response->set_message(success ? "Delete successful" : "Key not found");
        return Status::OK;
    }

    Status Exists(ServerContext* context,
                  const ExistsRequest* request,
                  ExistsResponse* response) override {
        std::cout << "Exists: key=" << request->key() << std::endl;

        bool exists = storage_engine_->Exists(request->key());
        response->set_exists(exists);
        return Status::OK;
    }

    Status BatchPut(ServerContext* context,
                    const BatchPutRequest* request,
                    BatchPutResponse* response) override {
        std::cout << "BatchPut: count=" << request->pairs_size() << std::endl;

        std::vector<KVItem> items;
        items.reserve(request->pairs_size());
        for (const auto& p : request->pairs()) {
            items.push_back({p.key(), p.value()});
        }

        size_t count = storage_engine_->BatchPut(items);
        response->set_succeeded_count(static_cast<uint32_t>(count));
        response->set_message("BatchPut completed");
        return Status::OK;
    }

    Status BatchGet(ServerContext* context,
                    const BatchGetRequest* request,
                    BatchGetResponse* response) override {
        std::cout << "BatchGet: count=" << request->keys_size() << std::endl;

        std::vector<std::string> keys(request->keys().begin(), request->keys().end());
        std::vector<KVItem> items;

        size_t found = storage_engine_->BatchGet(keys, items);

        for (const auto& item : items) {
            KVPair* pair = response->add_pairs();
            pair->set_key(item.key);
            pair->set_value(item.value);
        }
        response->set_found_count(static_cast<uint32_t>(found));
        response->set_missed_count(static_cast<uint32_t>(request->keys_size() - found));
        return Status::OK;
    }

    Status BatchDelete(ServerContext* context,
                       const BatchDeleteRequest* request,
                       BatchDeleteResponse* response) override {
        std::cout << "BatchDelete: count=" << request->keys_size() << std::endl;

        std::vector<std::string> keys(request->keys().begin(), request->keys().end());
        size_t deleted = storage_engine_->BatchDelete(keys);

        response->set_deleted_count(static_cast<uint32_t>(deleted));
        response->set_message("BatchDelete completed");
        return Status::OK;
    }

    Status CompareAndSwap(ServerContext* context,
                          const CASRequest* request,
                          CASResponse* response) override {
        std::cout << "CAS: key=" << request->key() << std::endl;

        std::string actual;
        bool key_existed = storage_engine_->Get(request->key(), actual);

        bool success = storage_engine_->CompareAndSwap(
            request->key(), request->expected_value(), request->new_value());

        response->set_success(success);
        response->set_key_existed(key_existed);
        if (!success && key_existed) {
            response->set_actual_value(actual);
            response->set_message("CAS failed: value mismatch");
        } else if (!success && !key_existed) {
            response->set_message("CAS failed: key not found");
        } else {
            response->set_message("CAS successful");
        }
        return Status::OK;
    }

private:
    std::unique_ptr<StorageEngine> storage_engine_;
};

// 存储引擎类型
enum class StorageType {
    FILE,     // 文件存储
    MEMORY    // 内存存储
};

// 创建存储引擎的工厂函数
std::unique_ptr<StorageEngine> CreateStorageEngine(StorageType type, 
                                                    const std::string& data_file) {
    switch (type) {
        case StorageType::FILE:
            return std::make_unique<FileStorageEngine>(data_file);
        case StorageType::MEMORY:
            return std::make_unique<MemoryStorageEngine>();
        default:
            return std::make_unique<FileStorageEngine>(data_file);
    }
}

} // namespace kvstore

void PrintUsage(const char* program) {
    std::cout << "Usage: " << program << " [options] [address] [data_file]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -m, --memory    Use in-memory storage (data lost on restart)" << std::endl;
    std::cout << "  -h, --help      Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  address         Server bind address (default: 0.0.0.0:50052)" << std::endl;
    std::cout << "  data_file       Data file path for file storage (default: kvstore.dat)" << std::endl;
}

int main(int argc, char** argv) {
    std::string address = "0.0.0.0:50052";
    std::string data_file = "kvstore.dat";
    kvstore::StorageType storage_type = kvstore::StorageType::FILE;

    // 解析命令行参数
    int arg_idx = 1;
    while (arg_idx < argc) {
        std::string arg = argv[arg_idx];
        if (arg == "-m" || arg == "--memory") {
            storage_type = kvstore::StorageType::MEMORY;
            arg_idx++;
        } else if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            // 非选项参数
            break;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // 剩余的位置参数
    int pos_arg = 0;
    while (arg_idx < argc && pos_arg < 2) {
        if (pos_arg == 0) {
            address = argv[arg_idx];
        } else if (pos_arg == 1 && storage_type == kvstore::StorageType::FILE) {
            data_file = argv[arg_idx];
        }
        arg_idx++;
        pos_arg++;
    }

    // 创建存储引擎
    auto storage_engine = kvstore::CreateStorageEngine(storage_type, data_file);
    
    // 创建服务
    kvstore::KVStoreServiceImpl service(std::move(storage_engine));

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    
    std::cout << "KVStore Server listening on " << address << std::endl;
    if (storage_type == kvstore::StorageType::FILE) {
        std::cout << "Storage type: FILE (data_file: " << data_file << ")" << std::endl;
    } else {
        std::cout << "Storage type: MEMORY (data will be lost on restart)" << std::endl;
    }
    
    server->Wait();
    
    return 0;
}
