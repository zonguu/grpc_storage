#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unistd.h>

#include "storage_engine.h"
#include "file_storage_engine.h"
#include "memory_storage_engine.h"

using namespace kvstore;

// 参数化测试：同时测试 File 和 Memory 两种引擎
class EngineTest : public ::testing::TestWithParam<std::function<std::unique_ptr<StorageEngine>()>> {
protected:
    std::unique_ptr<StorageEngine> engine_;
    std::filesystem::path temp_file_;

    void SetUp() override {
        engine_ = GetParam()();
        ASSERT_NE(engine_, nullptr);
    }

    void TearDown() override {
        if (engine_) {
            engine_->Close();
        }
        if (!temp_file_.empty() && std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }
};

// ===== L1-01 ~ L1-03: 基本 Put/Get/不存在 =====
TEST_P(EngineTest, BasicPutGet) {
    ASSERT_TRUE(engine_->Put("name", "alice"));
    std::string value;
    ASSERT_TRUE(engine_->Get("name", value));
    EXPECT_EQ(value, "alice");
}

TEST_P(EngineTest, OverwritePut) {
    ASSERT_TRUE(engine_->Put("key", "v1"));
    ASSERT_TRUE(engine_->Put("key", "v2"));
    std::string value;
    ASSERT_TRUE(engine_->Get("key", value));
    EXPECT_EQ(value, "v2");
}

TEST_P(EngineTest, GetNonExistent) {
    std::string value = "unchanged";
    EXPECT_FALSE(engine_->Get("nonexist", value));
    EXPECT_EQ(value, "unchanged");
}

// ===== L1-04 ~ L1-06: Delete / Exists =====
TEST_P(EngineTest, DeleteExisting) {
    ASSERT_TRUE(engine_->Put("k", "v"));
    EXPECT_TRUE(engine_->Delete("k"));
    std::string value;
    EXPECT_FALSE(engine_->Get("k", value));
}

TEST_P(EngineTest, DeleteNonExistent) {
    EXPECT_FALSE(engine_->Delete("nonexist"));
}

TEST_P(EngineTest, Exists) {
    EXPECT_FALSE(engine_->Exists("k"));
    ASSERT_TRUE(engine_->Put("k", "v"));
    EXPECT_TRUE(engine_->Exists("k"));
    EXPECT_FALSE(engine_->Exists("missing"));
}

// ===== L1-07 ~ L1-09: Batch 操作 =====
TEST_P(EngineTest, BatchPut) {
    std::vector<KVItem> items = {{"a", "1"}, {"b", "2"}, {"c", "3"}};
    EXPECT_EQ(engine_->BatchPut(items), 3u);

    std::string val;
    EXPECT_TRUE(engine_->Get("a", val)); EXPECT_EQ(val, "1");
    EXPECT_TRUE(engine_->Get("b", val)); EXPECT_EQ(val, "2");
    EXPECT_TRUE(engine_->Get("c", val)); EXPECT_EQ(val, "3");
}

TEST_P(EngineTest, BatchGetPartialMiss) {
    ASSERT_TRUE(engine_->Put("a", "1"));
    std::vector<KVItem> out;
    size_t found = engine_->BatchGet({"a", "missing", "b"}, out);
    EXPECT_EQ(found, 1u);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].key, "a");
    EXPECT_EQ(out[0].value, "1");
}

TEST_P(EngineTest, BatchDelete) {
    ASSERT_TRUE(engine_->Put("k1", "v1"));
    ASSERT_TRUE(engine_->Put("k2", "v2"));
    ASSERT_TRUE(engine_->Put("k3", "v3"));

    EXPECT_EQ(engine_->BatchDelete({"k1", "missing", "k3"}), 2u);

    std::string val;
    EXPECT_FALSE(engine_->Get("k1", val));
    EXPECT_TRUE(engine_->Get("k2", val)); EXPECT_EQ(val, "v2");
    EXPECT_FALSE(engine_->Get("k3", val));
}

// ===== L1-10 ~ L1-12: CAS =====
TEST_P(EngineTest, CASSuccess) {
    ASSERT_TRUE(engine_->Put("k", "old"));
    EXPECT_TRUE(engine_->CompareAndSwap("k", "old", "new"));
    std::string val;
    ASSERT_TRUE(engine_->Get("k", val));
    EXPECT_EQ(val, "new");
}

TEST_P(EngineTest, CASValueMismatch) {
    ASSERT_TRUE(engine_->Put("k", "old"));
    EXPECT_FALSE(engine_->CompareAndSwap("k", "wrong", "new"));
    std::string val;
    ASSERT_TRUE(engine_->Get("k", val));
    EXPECT_EQ(val, "old");
}

TEST_P(EngineTest, CASKeyNotFound) {
    EXPECT_FALSE(engine_->CompareAndSwap("missing", "", "new"));
}

// ===== L1-13: 空批量操作 =====
TEST_P(EngineTest, EmptyBatchOperations) {
    EXPECT_EQ(engine_->BatchPut({}), 0u);
    std::vector<KVItem> out;
    EXPECT_EQ(engine_->BatchGet({}, out), 0u);
    EXPECT_EQ(engine_->BatchDelete({}), 0u);
}

// ===== L1-14: 特殊字符 =====
TEST_P(EngineTest, SpecialCharacters) {
    std::string key = "key\twith\ttabs\n";
    std::string value = "val\nwith\nnewlines\t";
    ASSERT_TRUE(engine_->Put(key, value));
    std::string got;
    ASSERT_TRUE(engine_->Get(key, got));
    EXPECT_EQ(got, value);
}

// ===== L1-15: 大 value =====
TEST_P(EngineTest, LargeValue) {
    std::string big(1024 * 1024, 'x');  // 1MB
    ASSERT_TRUE(engine_->Put("big", big));
    std::string got;
    ASSERT_TRUE(engine_->Get("big", got));
    EXPECT_EQ(got, big);
    EXPECT_EQ(got.size(), 1024 * 1024u);
}

// ===== L1-16: 关闭后操作 =====
TEST_P(EngineTest, OperationsAfterClose) {
    ASSERT_TRUE(engine_->Put("k", "v"));
    engine_->Close();
    EXPECT_FALSE(engine_->Put("k2", "v2"));
    std::string val;
    EXPECT_FALSE(engine_->Get("k", val));
    EXPECT_FALSE(engine_->Exists("k"));
}

// ===== File 引擎专属用例 =====
class FileEngineTest : public ::testing::Test {
protected:
    std::filesystem::path temp_file_;
    std::unique_ptr<StorageEngine> engine_;

    void SetUp() override {
        temp_file_ = std::filesystem::temp_directory_path() / "kvstore_eng_test_XXXXXX";
        char tmpl[] = "/tmp/kvstore_eng_test_XXXXXX";
        int fd = mkstemp(tmpl);
        if (fd >= 0) close(fd);
        temp_file_ = tmpl;
        engine_ = std::make_unique<FileStorageEngine>(temp_file_.string());
    }

    void TearDown() override {
        if (engine_) engine_->Close();
        if (std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }
};

// L1-F01: 持久化
TEST_F(FileEngineTest, Persistence) {
    ASSERT_TRUE(engine_->Put("k", "v"));
    engine_->Close();

    // 重新打开同一文件
    auto engine2 = std::make_unique<FileStorageEngine>(temp_file_.string());
    std::string val;
    EXPECT_TRUE(engine2->Get("k", val));
    EXPECT_EQ(val, "v");
}

// L1-F02: 并发文件加载（另一个进程/线程持有文件时，引擎能正常加载）
// 注：FileStorageEngine 当前是加载到内存后操作，文件句柄不长期持有。
// 这个测试验证的是：外部修改文件后，新实例能看到最新数据。
TEST_F(FileEngineTest, ExternalFileModification) {
    ASSERT_TRUE(engine_->Put("k", "v1"));
    engine_->Close();

    // 模拟外部进程直接修改文件
    {
        std::ofstream ofs(temp_file_.string(), std::ios::trunc);
        ofs << "external_key\texternal_value\n";
    }

    auto engine2 = std::make_unique<FileStorageEngine>(temp_file_.string());
    std::string val;
    EXPECT_TRUE(engine2->Get("external_key", val));
    EXPECT_EQ(val, "external_value");
    EXPECT_FALSE(engine2->Get("k", val));
}

// ===== 参数化测试实例化 =====

// Memory 引擎工厂
auto MemoryEngineFactory = []() -> std::unique_ptr<StorageEngine> {
    return std::make_unique<MemoryStorageEngine>();
};

// File 引擎工厂
auto FileEngineFactory = []() -> std::unique_ptr<StorageEngine> {
    char tmpl[] = "/tmp/kvstore_param_test_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd >= 0) close(fd);
    return std::make_unique<FileStorageEngine>(tmpl);
};

INSTANTIATE_TEST_SUITE_P(
    EngineBackends,
    EngineTest,
    ::testing::Values(MemoryEngineFactory, FileEngineFactory));
