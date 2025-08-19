#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "file_operations.h"
#include <filesystem>
#include <fstream>

using namespace mag;

class FileOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        file_tool_ = std::make_unique<FileTool>();
        test_dir_ = "test_output";
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::unique_ptr<FileTool> file_tool_;
    std::string test_dir_;
};

TEST_F(FileOperationsTest, DryRunNewFile) {
    std::string path = test_dir_ + "/new_file.txt";
    std::string content = "Hello, World!";
    
    DryRunResult result = file_tool_->dry_run(path, content);
    
    EXPECT_TRUE(result.success);
    EXPECT_THAT(result.description, testing::HasSubstr("[DRY-RUN]"));
    EXPECT_THAT(result.description, testing::HasSubstr("create new file"));
    EXPECT_THAT(result.description, testing::HasSubstr(path));
    EXPECT_EQ(result.error_message, "");
}

TEST_F(FileOperationsTest, DryRunExistingFile) {
    std::string path = test_dir_ + "/existing_file.txt";
    std::string content = "Hello, World!";
    
    // Create the file first
    std::ofstream file(path);
    file << "original content";
    file.close();
    
    DryRunResult result = file_tool_->dry_run(path, content);
    
    EXPECT_TRUE(result.success);
    EXPECT_THAT(result.description, testing::HasSubstr("[DRY-RUN]"));
    EXPECT_THAT(result.description, testing::HasSubstr("overwrite existing file"));
    EXPECT_THAT(result.description, testing::HasSubstr(path));
    EXPECT_EQ(result.error_message, "");
}

TEST_F(FileOperationsTest, ApplySuccess) {
    std::string path = test_dir_ + "/apply_test.txt";
    std::string content = "Test content for apply operation";
    
    ApplyResult result = file_tool_->apply(path, content);
    
    EXPECT_TRUE(result.success);
    EXPECT_THAT(result.description, testing::HasSubstr("[APPLIED]"));
    EXPECT_THAT(result.description, testing::HasSubstr("Successfully wrote"));
    EXPECT_THAT(result.description, testing::HasSubstr(path));
    EXPECT_EQ(result.error_message, "");
    
    // Verify file was actually created with correct content
    EXPECT_TRUE(std::filesystem::exists(path));
    std::ifstream file(path);
    std::string file_content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    EXPECT_EQ(content, file_content);
}

TEST_F(FileOperationsTest, ApplyWithDirectoryCreation) {
    std::string path = test_dir_ + "/subdir/deep/apply_test.txt";
    std::string content = "Test content with directory creation";
    
    ApplyResult result = file_tool_->apply(path, content);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(path));
    
    // Verify content
    std::ifstream file(path);
    std::string file_content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    EXPECT_EQ(content, file_content);
}