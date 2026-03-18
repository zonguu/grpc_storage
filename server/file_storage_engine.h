#pragma once

#include "storage_engine.h"

#include <string>
#include <mutex>
#include <unordered_map>

namespace kvstore {

// 基于文件的存储引擎实现
class FileStorageEngine : public StorageEngine {
public:
    explicit FileStorageEngine(const std::string& filename);
    ~FileStorageEngine() override;

    // 禁止拷贝和移动
    FileStorageEngine(const FileStorageEngine&) = delete;
    FileStorageEngine& operator=(const FileStorageEngine&) = delete;

    // StorageEngine 接口实现
    bool Put(const std::string& key, const std::string& value) override;
    bool Get(const std::string& key, std::string& value) override;
    bool Delete(const std::string& key) override;
    void Close() override;

private:
    // 从文件加载数据到内存
    void LoadFromFile();
    
    // 将内存数据保存到文件
    void SaveToFile();

    std::string filename_;
    std::unordered_map<std::string, std::string> data_;
    std::mutex mutex_;
    bool closed_ = false;
};

} // namespace kvstore
