#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "llm_client.h"

using namespace mag;

class LLMClientTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LLMClientTest, ProviderConstruction) {
    // Test that we can construct clients with different provider strings
    // Note: These will fail at runtime without API keys, but should compile
    EXPECT_NO_THROW(LLMClient("openai", "fake-key"));
    EXPECT_NO_THROW(LLMClient("anthropic", "fake-key"));
    EXPECT_NO_THROW(LLMClient("gemini", "fake-key"));
    EXPECT_NO_THROW(LLMClient("mistral", "fake-key"));
}

TEST_F(LLMClientTest, CustomModelSelection) {
    // Test custom model specification
    EXPECT_NO_THROW(LLMClient("openai", "fake-key", "gpt-4"));
    EXPECT_NO_THROW(LLMClient("anthropic", "fake-key", "claude-3-opus-20240229"));
    EXPECT_NO_THROW(LLMClient("gemini", "fake-key", "gemini-pro-vision"));
    EXPECT_NO_THROW(LLMClient("mistral", "fake-key", "mistral-large-latest"));
}

TEST_F(LLMClientTest, UnsupportedProvider) {
    // Test that unsupported providers throw exceptions
    EXPECT_THROW(LLMClient("unsupported", "fake-key"), std::runtime_error);
}

// Note: We can't easily test actual API calls without mocking curl or having real API keys
// These tests focus on the structure and configuration aspects