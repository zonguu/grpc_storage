#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "kvstore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
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

class KVStoreClient {
public:
    KVStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

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

    bool Delete(const std::string& key) {
        DeleteRequest request;
        request.set_key(key);

        DeleteResponse response;
        ClientContext context;

        Status status = stub_->Delete(&context, request, &response);

        if (status.ok()) {
            std::cout << "Delete " << (response.success() ? "succeeded" : "failed")
                      << ": " << response.message() << std::endl;
            return response.success();
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool Exists(const std::string& key) {
        ExistsRequest request;
        request.set_key(key);

        ExistsResponse response;
        ClientContext context;

        Status status = stub_->Exists(&context, request, &response);

        if (status.ok()) {
            std::cout << "Exists: key=" << key
                      << " -> " << (response.exists() ? "yes" : "no") << std::endl;
            return response.exists();
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool BatchPut(const std::vector<std::pair<std::string, std::string>>& kvs) {
        BatchPutRequest request;
        for (const auto& kv : kvs) {
            KVPair* pair = request.add_pairs();
            pair->set_key(kv.first);
            pair->set_value(kv.second);
        }

        BatchPutResponse response;
        ClientContext context;

        Status status = stub_->BatchPut(&context, request, &response);

        if (status.ok()) {
            std::cout << "BatchPut succeeded: " << response.succeeded_count()
                      << "/" << kvs.size() << " " << response.message() << std::endl;
            return true;
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool BatchGet(const std::vector<std::string>& keys) {
        BatchGetRequest request;
        for (const auto& key : keys) {
            request.add_keys(key);
        }

        BatchGetResponse response;
        ClientContext context;

        Status status = stub_->BatchGet(&context, request, &response);

        if (status.ok()) {
            std::cout << "BatchGet result: found=" << response.found_count()
                      << ", missed=" << response.missed_count() << std::endl;
            for (const auto& p : response.pairs()) {
                std::cout << "  " << p.key() << "=" << p.value() << std::endl;
            }
            return true;
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool BatchDelete(const std::vector<std::string>& keys) {
        BatchDeleteRequest request;
        for (const auto& key : keys) {
            request.add_keys(key);
        }

        BatchDeleteResponse response;
        ClientContext context;

        Status status = stub_->BatchDelete(&context, request, &response);

        if (status.ok()) {
            std::cout << "BatchDelete succeeded: " << response.deleted_count()
                      << "/" << keys.size() << " " << response.message() << std::endl;
            return true;
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool CompareAndSwap(const std::string& key,
                        const std::string& expected,
                        const std::string& new_value) {
        CASRequest request;
        request.set_key(key);
        request.set_expected_value(expected);
        request.set_new_value(new_value);

        CASResponse response;
        ClientContext context;

        Status status = stub_->CompareAndSwap(&context, request, &response);

        if (status.ok()) {
            if (response.success()) {
                std::cout << "CAS succeeded: " << response.message() << std::endl;
            } else {
                std::cout << "CAS failed: " << response.message();
                if (response.key_existed()) {
                    std::cout << ", actual_value=" << response.actual_value();
                }
                std::cout << std::endl;
            }
            return response.success();
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
    std::cout << "  " << program << " put <key> <value>            - Store key-value pair" << std::endl;
    std::cout << "  " << program << " get <key>                    - Retrieve value by key" << std::endl;
    std::cout << "  " << program << " delete <key>                 - Delete key" << std::endl;
    std::cout << "  " << program << " exists <key>                 - Check if key exists" << std::endl;
    std::cout << "  " << program << " batchput k1 v1 k2 v2 ...     - Batch store" << std::endl;
    std::cout << "  " << program << " batchget k1 k2 ...           - Batch retrieve" << std::endl;
    std::cout << "  " << program << " batchdelete k1 k2 ...        - Batch delete" << std::endl;
    std::cout << "  " << program << " cas <key> <expected> <new>   - Compare and swap" << std::endl;
    std::cout << "  " << program << " <key> <value>                - Store key-value pair (shorthand)" << std::endl;
}

int main(int argc, char** argv) {
    std::string server_address = "localhost:50052";

    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    KVStoreClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::string cmd = argv[1];

    if (cmd == "put" && argc == 4) {
        client.Put(argv[2], argv[3]);
    } else if (cmd == "get" && argc == 3) {
        std::string value;
        client.Get(argv[2], value);
    } else if (cmd == "delete" && argc == 3) {
        client.Delete(argv[2]);
    } else if (cmd == "exists" && argc == 3) {
        client.Exists(argv[2]);
    } else if (cmd == "batchput" && argc >= 4 && (argc - 2) % 2 == 0) {
        std::vector<std::pair<std::string, std::string>> kvs;
        for (int i = 2; i < argc; i += 2) {
            kvs.emplace_back(argv[i], argv[i + 1]);
        }
        client.BatchPut(kvs);
    } else if (cmd == "batchget" && argc >= 3) {
        std::vector<std::string> keys;
        for (int i = 2; i < argc; ++i) {
            keys.push_back(argv[i]);
        }
        client.BatchGet(keys);
    } else if (cmd == "batchdelete" && argc >= 3) {
        std::vector<std::string> keys;
        for (int i = 2; i < argc; ++i) {
            keys.push_back(argv[i]);
        }
        client.BatchDelete(keys);
    } else if (cmd == "cas" && argc == 5) {
        client.CompareAndSwap(argv[2], argv[3], argv[4]);
    } else if (argc == 3) {
        // 简写形式: client <key> <value>
        client.Put(argv[1], argv[2]);
    } else {
        PrintUsage(argv[0]);
        return 1;
    }

    return 0;
}
