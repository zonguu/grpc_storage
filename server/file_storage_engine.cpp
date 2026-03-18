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
    SaveToFile();
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
        SaveToFile();
        return true;
    }
    return false;
}

void FileStorageEngine::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!closed_) {
        SaveToFile();
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

void FileStorageEngine::SaveToFile() {
    std::ofstream file(filename_, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename_ << std::endl;
        return;
    }

    for (const auto& [key, value] : data_) {
        file << key << '\t' << value << '\n';
    }
    file.close();
}

} // namespace kvstore
