#include "memory_storage_engine.h"

#include <iostream>

namespace kvstore {

bool MemoryStorageEngine::Put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
    return true;
}

bool MemoryStorageEngine::Get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool MemoryStorageEngine::Delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(key) > 0;
}

void MemoryStorageEngine::Close() {
    // 内存引擎不需要特殊关闭操作
    std::cout << "Memory storage engine closed. " << data_.size() << " entries lost." << std::endl;
}

void MemoryStorageEngine::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
}

size_t MemoryStorageEngine::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

} // namespace kvstore
