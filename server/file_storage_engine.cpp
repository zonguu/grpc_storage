#include "file_storage_engine.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace kvstore {

FileStorageEngine::FileStorageEngine(const std::string& filename) 
    : filename_(filename) {
    LoadFromFile();
}

FileStorageEngine::~FileStorageEngine() {
    Close();
}

bool FileStorageEngine::Put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    data_[key] = value;
    SaveToFileLocked();
    return true;
}

bool FileStorageEngine::Get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    auto it = data_.find(key);
    if (it != data_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool FileStorageEngine::Delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    auto it = data_.find(key);
    if (it != data_.end()) {
        data_.erase(it);
        SaveToFileLocked();
        return true;
    }
    return false;
}

bool FileStorageEngine::Exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    return data_.find(key) != data_.end();
}

size_t FileStorageEngine::BatchPut(const std::vector<KVItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return 0;
    }
    for (const auto& item : items) {
        data_[item.key] = item.value;
    }
    SaveToFileLocked();
    return items.size();
}

size_t FileStorageEngine::BatchGet(const std::vector<std::string>& keys,
                                   std::vector<KVItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return 0;
    }
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

size_t FileStorageEngine::BatchDelete(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return 0;
    }
    size_t deleted = 0;
    for (const auto& key : keys) {
        deleted += data_.erase(key);
    }
    if (deleted > 0) {
        SaveToFileLocked();
    }
    return deleted;
}

bool FileStorageEngine::CompareAndSwap(const std::string& key,
                                       const std::string& expected_value,
                                       const std::string& new_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    auto it = data_.find(key);
    if (it != data_.end() && it->second == expected_value) {
        it->second = new_value;
        SaveToFileLocked();
        return true;
    }
    return false;
}

void FileStorageEngine::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!closed_) {
        SaveToFileLocked();
        closed_ = true;
    }
}

void FileStorageEngine::LoadFromFile() {
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cout << "Data file not found, starting with empty store." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 格式: key\tvalue
        size_t pos = line.find('\t');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            data_[key] = value;
        }
    }
    file.close();
    std::cout << "Loaded " << data_.size() << " entries from file." << std::endl;
}

void FileStorageEngine::SaveToFileLocked() {
    std::ofstream file(filename_, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename_ << std::endl;
        return;
    }

    for (const auto& [k, v] : data_) {
        file << k << '\t' << v << '\n';
    }
    file.close();
}

} // namespace kvstore
