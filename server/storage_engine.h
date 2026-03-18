#pragma once

#include <string>
#include <memory>
#include <functional>

namespace kvstore {

// 存储引擎抽象接口
// 所有具体存储实现都需要继承此接口
class StorageEngine {
public:
    virtual ~StorageEngine() = default;

    // 写入键值对
    // @param key: 键
    // @param value: 值
    // @return: 是否成功
    virtual bool Put(const std::string& key, const std::string& value) = 0;

    // 读取键值对
    // @param key: 键
    // @param value: 输出参数，返回找到的值
    // @return: 是否找到
    virtual bool Get(const std::string& key, std::string& value) = 0;

    // 删除键值对（可选实现）
    // @param key: 键
    // @return: 是否成功删除（键存在且删除成功）
    virtual bool Delete(const std::string& key) = 0;

    // 关闭存储引擎，释放资源
    virtual void Close() = 0;
};

// 存储引擎创建函数类型
using StorageEngineFactory = std::function<std::unique_ptr<StorageEngine>()>;

} // namespace kvstore
