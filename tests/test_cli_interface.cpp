#include <gtest/gtest.h>
#include "coordinator.h"
#include "interfaces/llm_client_interface.h"
#include "interfaces/file_client_interface.h"
#include <memory>
#include <vector>

namespace mag {

// Test implementations (reusing the working ones from coordinator interfaces test)
class TestLLMClient : public ILLMClient {
public:
    WriteFileCommand request_plan(const std::string& user_prompt) override {
        plan_requests.push_back(user_prompt);
        return mock_plan_response;
    }
    
    GenericCommand request_generic_plan(const std::string& user_prompt) override {
        generic_plan_requests.push_back(user_prompt);
        return mock_generic_response;
    }
    
    std::string request_chat(const std::string& user_prompt) override {
        chat_requests.push_back(user_prompt);
        return mock_chat_response;
    }
    
    void set_provider(const std::string& provider_name) override {
        provider_calls.push_back(provider_name);
        current_provider = provider_name;
    }
    
    std::string get_current_provider() const override {
        return current_provider;
    }
    
    // Test data
    std::vector<std::string> plan_requests;
    std::vector<std::string> generic_plan_requests;
    std::vector<std::string> chat_requests;
    std::vector<std::string> provider_calls;
    std::string current_provider = "test_provider";
    WriteFileCommand mock_plan_response;
    GenericCommand mock_generic_response;
    std::string mock_chat_response = "Test chat response";
};

class TestFileClient : public IFileClient {
public:
    DryRunResult dry_run(const WriteFileCommand& command) override {
        dry_run_calls.push_back(command);
        return mock_dry_run_response;
    }
    
    ApplyResult apply(const WriteFileCommand& command) override {
        apply_calls.push_back(command);
        return mock_apply_response;
    }
    
    // Test data
    std::vector<WriteFileCommand> dry_run_calls;
    std::vector<WriteFileCommand> apply_calls;
    DryRunResult mock_dry_run_response;
    ApplyResult mock_apply_response;
};


class CLIInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test implementations and store raw pointers before moving
        auto test_llm = std::make_unique<TestLLMClient>();
        auto test_file = std::make_unique<TestFileClient>();
        
        llm_client_ptr = test_llm.get();
        file_client_ptr = test_file.get();
        
        // Setup mock responses
        llm_client_ptr->mock_plan_response.path = "tests/test_file.txt";
        llm_client_ptr->mock_plan_response.content = "Test content";
        llm_client_ptr->mock_plan_response.command = "write";
        
        file_client_ptr->mock_dry_run_response.success = true;
        file_client_ptr->mock_dry_run_response.description = "Test dry run";
        
        file_client_ptr->mock_apply_response.success = true;
        file_client_ptr->mock_apply_response.description = "Test apply success";
        
        // Create coordinator with injected test implementations
        coordinator_ = std::make_unique<Coordinator>(
            std::move(test_llm),
            std::move(test_file)
        );
        
        // Setup test todos
        setupTestTodos();
    }
    
    void setupTestTodos() {
        auto& todo_manager = coordinator_->get_todo_manager();
        
        // Add some test todos
        todo_manager.add_todo("First task", "Description of first task");
        todo_manager.add_todo("Second task", "Description of second task");
        todo_manager.add_todo("Third task", "Description of third task");
        todo_manager.add_todo("Fourth task", "Description of fourth task");
    }
    
    std::unique_ptr<Coordinator> coordinator_;
    TestLLMClient* llm_client_ptr;
    TestFileClient* file_client_ptr;
};

// Test execute command parsing
TEST_F(CLIInterfaceTest, ExecuteCommandParsing) {
    auto& todo_manager = coordinator_->get_todo_manager();
    
    // Test execute all (will fail due to empty LLM response, but should attempt all)
    coordinator_->execute_todos();
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), 4); // All todos should request LLM plan
    
    // Reset for next test
    llm_client_ptr->plan_requests.clear();
    setupTestTodos();
    
    // Test execute next
    coordinator_->execute_next_todo();
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), 1); // Only one todo should request LLM plan
    
    // Test execute until (should stop at ID 3, so execute 1 and 2)
    coordinator_->execute_todos_until(3);
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), 2); // Should attempt todos 1 and 2
    
    // Reset for range test
    llm_client_ptr->plan_requests.clear();
    setupTestTodos();
    
    // Test execute range (should execute todos 2, 3, 4)
    coordinator_->execute_todos_range(2, 4);
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), 3); // Should attempt todos 2, 3, 4
}

TEST_F(CLIInterfaceTest, ExecutionStateManagement) {
    // Test initial state
    EXPECT_EQ(coordinator_->get_execution_state(), Coordinator::ExecutionState::STOPPED);
    
    // The execution state management requires execution to be in progress.
    // Since we can't easily simulate that without complex threading,
    // we'll test that the methods don't crash when called inappropriately
    
    // These should not crash and should print appropriate messages
    coordinator_->pause_execution();
    coordinator_->resume_execution(); 
    coordinator_->stop_execution();
    coordinator_->cancel_execution();
    
    // State should remain STOPPED since no execution was in progress
    EXPECT_EQ(coordinator_->get_execution_state(), Coordinator::ExecutionState::STOPPED);
}

TEST_F(CLIInterfaceTest, ExecuteSingleTodoWithValidResponse) {
    auto& todo_manager = coordinator_->get_todo_manager();
    auto todos = todo_manager.list_todos(false);
    
    ASSERT_FALSE(todos.empty());
    
    // Setup valid LLM response
    llm_client_ptr->mock_plan_response.path = "tests/test_output.txt";
    llm_client_ptr->mock_plan_response.command = "write";
    
    // Execute single todo  
    coordinator_->execute_single_todo(todos[0]);
    
    // Verify LLM was called
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), 1);
    // Verify the prompt was constructed properly
    EXPECT_FALSE(llm_client_ptr->plan_requests[0].empty());
    
    // Since we have valid path now, file operations should be attempted
    EXPECT_EQ(file_client_ptr->dry_run_calls.size(), 1);
    EXPECT_EQ(file_client_ptr->apply_calls.size(), 1);
}

TEST_F(CLIInterfaceTest, ExecuteInvalidRange) {
    // Test invalid range should not crash
    try {
        coordinator_->execute_todos_range(10, 20); // IDs that don't exist
        // Should not crash, might execute nothing or handle gracefully
    } catch (const std::exception& e) {
        // Exception is acceptable for invalid range
    }
}

TEST_F(CLIInterfaceTest, ExecuteUntilBeyondRange) {
    auto& todo_manager = coordinator_->get_todo_manager();
    auto initial_count = todo_manager.list_todos(false).size();
    
    // Execute until beyond existing todos
    coordinator_->execute_todos_until(100);
    
    // Should attempt to execute all existing todos (they'll fail due to empty LLM response)
    EXPECT_EQ(llm_client_ptr->plan_requests.size(), initial_count);
}

// Test slash command parsing would go here if we had a way to mock CLI interface
// For now, we focus on the coordinator execution logic which is the core functionality

} // namespace mag