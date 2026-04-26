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

bool MemoryStorageEngine::Exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

size_t MemoryStorageEngine::BatchPut(const std::vector<KVItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& item : items) {
        data_[item.key] = item.value;
    }
    return items.size();
}

size_t MemoryStorageEngine::BatchGet(const std::vector<std::string>& keys,
                                     std::vector<KVItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    items.clear();
    items.reserve(keys.size());
    size_t found = 0;
    for (const auto& key : keys) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            items.push_back({it->first, it->second});
            ++found;
        }
    }
    return found;
}

size_t MemoryStorageEngine::BatchDelete(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t deleted = 0;
    for (const auto& key : keys) {
        deleted += data_.erase(key);
    }
    return deleted;
}

bool MemoryStorageEngine::CompareAndSwap(const std::string& key,
                                         const std::string& expected_value,
                                         const std::string& new_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end() && it->second == expected_value) {
        it->second = new_value;
        return true;
    }
    return false;
}

void MemoryStorageEngine::Close() {
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
