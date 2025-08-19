#include <gtest/gtest.h>
#include "policy.h"

using namespace mag;

class PolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_checker_ = std::make_unique<PolicyChecker>();
    }
    
    std::unique_ptr<PolicyChecker> policy_checker_;
};

TEST_F(PolicyTest, AllowedPaths) {
    EXPECT_TRUE(policy_checker_->is_allowed("src/main.cpp"));
    EXPECT_TRUE(policy_checker_->is_allowed("tests/test_main.cpp"));
    EXPECT_TRUE(policy_checker_->is_allowed("docs/README.md"));
}

TEST_F(PolicyTest, DisallowedPaths) {
    EXPECT_FALSE(policy_checker_->is_allowed("../etc/passwd"));
    EXPECT_FALSE(policy_checker_->is_allowed("/etc/passwd"));
    EXPECT_FALSE(policy_checker_->is_allowed("bin/executable"));
    EXPECT_FALSE(policy_checker_->is_allowed("config/secret.txt"));
}

TEST_F(PolicyTest, RelativePathTraversal) {
    EXPECT_FALSE(policy_checker_->is_allowed("src/../../../etc/passwd"));
    EXPECT_FALSE(policy_checker_->is_allowed("../src/main.cpp"));
}