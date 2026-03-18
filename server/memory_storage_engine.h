#pragma once

#include "storage_engine.h"

#include <string>
#include <mutex>
#include <unordered_map>

namespace kvstore {

// 纯内存存储引擎实现（不持久化，重启数据丢失）
// 适用于缓存场景或测试
class MemoryStorageEngine : public StorageEngine {
public:
    MemoryStorageEngine() = default;
    ~MemoryStorageEngine() override = default;

    // 禁止拷贝
    MemoryStorageEngine(const MemoryStorageEngine&) = delete;
    MemoryStorageEngine& operator=(const MemoryStorageEngine&) = delete;

    // StorageEngine 接口实现
    bool Put(const std::string& key, const std::string& value) override;
    bool Get(const std::string& key, std::string& value) override;
    bool Delete(const std::string& key) override;
    void Close() override;

    // 内存引擎特有：清空所有数据
    void Clear();

    // 内存引擎特有：获取当前键值对数量
    size_t Size() const;

private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};

} // namespace kvstore
