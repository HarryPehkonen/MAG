#include <gtest/gtest.h>
#include "message.h"

using namespace mag;

class MessageTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MessageTest, WriteFileCommandSerialization) {
    WriteFileCommand cmd;
    cmd.command = "WriteFile";
    cmd.path = "src/test.cpp";
    cmd.content = "#include <iostream>\nint main() { return 0; }";
    
    std::string serialized = MessageHandler::serialize_command(cmd);
    EXPECT_FALSE(serialized.empty());
    
    WriteFileCommand deserialized = MessageHandler::deserialize_command(serialized);
    EXPECT_EQ(cmd.command, deserialized.command);
    EXPECT_EQ(cmd.path, deserialized.path);
    EXPECT_EQ(cmd.content, deserialized.content);
}

TEST_F(MessageTest, DryRunResultSerialization) {
    DryRunResult result;
    result.description = "[DRY-RUN] Will create new file 'test.txt' with 42 bytes.";
    result.success = true;
    result.error_message = "";
    
    std::string serialized = MessageHandler::serialize_dry_run_result(result);
    EXPECT_FALSE(serialized.empty());
    
    DryRunResult deserialized = MessageHandler::deserialize_dry_run_result(serialized);
    EXPECT_EQ(result.description, deserialized.description);
    EXPECT_EQ(result.success, deserialized.success);
    EXPECT_EQ(result.error_message, deserialized.error_message);
}

TEST_F(MessageTest, ApplyResultSerialization) {
    ApplyResult result;
    result.description = "[APPLIED] Successfully wrote 42 bytes to 'test.txt'.";
    result.success = true;
    result.error_message = "";
    
    std::string serialized = MessageHandler::serialize_apply_result(result);
    EXPECT_FALSE(serialized.empty());
    
    ApplyResult deserialized = MessageHandler::deserialize_apply_result(serialized);
    EXPECT_EQ(result.description, deserialized.description);
    EXPECT_EQ(result.success, deserialized.success);
    EXPECT_EQ(result.error_message, deserialized.error_message);
}