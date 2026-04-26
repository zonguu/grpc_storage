#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <utility>

namespace kvstore {

// 键值对结构，用于批量接口
struct KVItem {
    std::string key;
    std::string value;
};

// 存储引擎抽象接口
// 所有具体存储实现都需要继承此接口
class StorageEngine {
public:
    virtual ~StorageEngine() = default;

    // 写入键值对
    virtual bool Put(const std::string& key, const std::string& value) = 0;

    // 读取键值对
    virtual bool Get(const std::string& key, std::string& value) = 0;

    // 删除键值对
    virtual bool Delete(const std::string& key) = 0;

    // 检查键是否存在
    virtual bool Exists(const std::string& key) = 0;

    // 批量写入键值对（高性能：底层实现应只加锁一次、只刷盘一次）
    // @return: 成功写入的数量
    virtual size_t BatchPut(const std::vector<KVItem>& items) = 0;

    // 批量读取键值对（高性能：底层实现应只加锁一次）
    // @param keys: 要查询的键列表
    // @param[out] items: 返回找到的键值对（不存在的键不会出现在结果中）
    // @return: 找到的数量
    virtual size_t BatchGet(const std::vector<std::string>& keys,
                            std::vector<KVItem>& items) = 0;

    // 批量删除键值对（高性能：底层实现应只加锁一次、只刷盘一次）
    // @return: 成功删除的数量
    virtual size_t BatchDelete(const std::vector<std::string>& keys) = 0;

    // 比较并交换（原子操作）
    // 仅当 key 存在且当前值等于 expected_value 时，才将其替换为 new_value
    // @return: 是否交换成功
    virtual bool CompareAndSwap(const std::string& key,
                                const std::string& expected_value,
                                const std::string& new_value) = 0;

    // 关闭存储引擎，释放资源
    virtual void Close() = 0;
};

// 存储引擎创建函数类型
using StorageEngineFactory = std::function<std::unique_ptr<StorageEngine>()>;

} // namespace kvstore
