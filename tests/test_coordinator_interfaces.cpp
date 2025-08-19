#include <gtest/gtest.h>
#include "coordinator.h"
#include "interfaces/llm_client_interface.h"
#include "interfaces/file_client_interface.h"
#include <memory>
#include <vector>

namespace mag {

// Test implementations (not using GMock to avoid segfault issues)
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

class CoordinatorInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test implementations and store raw pointers before moving
        auto test_llm = std::make_unique<TestLLMClient>();
        auto test_file = std::make_unique<TestFileClient>();
        
        llm_client_ptr = test_llm.get();
        file_client_ptr = test_file.get();
        
        // Create coordinator with injected test implementations
        coordinator = std::make_unique<Coordinator>(
            std::move(test_llm),
            std::move(test_file)
        );
    }
    
    void TearDown() override {
        // Reset coordinator first to ensure proper cleanup order
        coordinator.reset();
    }
    
    TestLLMClient* llm_client_ptr = nullptr; // Raw pointer for verifying calls
    TestFileClient* file_client_ptr = nullptr; // Raw pointer for verifying calls
    std::unique_ptr<Coordinator> coordinator;
};

TEST_F(CoordinatorInterfaceTest, ChatModeCallsLLMClient) {
    std::string user_input = "Create a hello world script";
    std::string llm_response = "I'll help you with that! add_todo(\"Create hello world\", \"Python script\")";
    
    // Set up mock response
    llm_client_ptr->mock_chat_response = llm_response;
    
    // Execute
    coordinator->set_chat_mode(true);
    coordinator->run(user_input);
    
    // Verify LLM client was called
    ASSERT_EQ(llm_client_ptr->chat_requests.size(), 1);
    EXPECT_EQ(llm_client_ptr->chat_requests[0], user_input);
    
    // Verify todo was added from LLM response
    auto todos = coordinator->get_todo_manager().list_todos();
    ASSERT_EQ(todos.size(), 1);
    EXPECT_EQ(todos[0].title, "Create hello world");
    EXPECT_EQ(todos[0].description, "Python script");
}

TEST_F(CoordinatorInterfaceTest, FileOperationCallsFileClient) {
    WriteFileCommand test_command;
    test_command.command = "WriteFile";
    test_command.path = "tests/test_hello.py";  // Use allowed directory
    test_command.content = "print('Hello World')";
    
    DryRunResult dry_run_result;
    dry_run_result.success = true;
    dry_run_result.description = "Will create tests/test_hello.py with 22 bytes";
    
    ApplyResult apply_result;
    apply_result.success = true;
    apply_result.description = "Successfully created tests/test_hello.py";
    
    // Set up mock responses
    llm_client_ptr->mock_plan_response = test_command;
    file_client_ptr->mock_dry_run_response = dry_run_result;
    file_client_ptr->mock_apply_response = apply_result;
    
    // Execute single todo to test file operations
    TodoItem test_todo;
    test_todo.id = 1;
    test_todo.title = "Create hello world script";
    test_todo.description = "Python script";
    test_todo.status = TodoStatus::PENDING;
    
    coordinator->execute_single_todo(test_todo);
    
    // Verify calls were made
    ASSERT_EQ(llm_client_ptr->plan_requests.size(), 1);
    ASSERT_EQ(file_client_ptr->dry_run_calls.size(), 1);
    ASSERT_EQ(file_client_ptr->apply_calls.size(), 1);
    
    // Verify the content passed through correctly
    EXPECT_EQ(file_client_ptr->dry_run_calls[0].path, "tests/test_hello.py");
    EXPECT_EQ(file_client_ptr->apply_calls[0].path, "tests/test_hello.py");
}

TEST_F(CoordinatorInterfaceTest, ProviderSwitchingCallsLLMClient) {
    coordinator->set_provider("claude");
    
    // Verify the provider was set
    ASSERT_EQ(llm_client_ptr->provider_calls.size(), 1);
    EXPECT_EQ(llm_client_ptr->provider_calls[0], "claude");
    EXPECT_EQ(llm_client_ptr->current_provider, "claude");
}

TEST_F(CoordinatorInterfaceTest, ExecuteSingleTodoUsesInterfaces) {
    WriteFileCommand test_command;
    test_command.command = "WriteFile";
    test_command.path = "src/hello_world.py";  // Use allowed directory
    test_command.content = "print('Hello World')";
    
    DryRunResult dry_run_result;
    dry_run_result.success = true;
    dry_run_result.description = "Will create src/hello_world.py";
    
    ApplyResult apply_result;
    apply_result.success = true;
    apply_result.description = "Successfully created src/hello_world.py";
    
    // Set up mock responses
    llm_client_ptr->mock_plan_response = test_command;
    file_client_ptr->mock_dry_run_response = dry_run_result;
    file_client_ptr->mock_apply_response = apply_result;
    
    // Create a test todo
    TodoItem todo;
    todo.id = 1;
    todo.title = "Create hello world";
    todo.description = "Python script";
    todo.status = TodoStatus::PENDING;
    
    // Execute single todo
    coordinator->execute_single_todo(todo);
    
    // Verify all interfaces were called correctly
    ASSERT_EQ(llm_client_ptr->plan_requests.size(), 1);
    ASSERT_EQ(file_client_ptr->dry_run_calls.size(), 1);
    ASSERT_EQ(file_client_ptr->apply_calls.size(), 1);
    
    // Verify the prompt was constructed correctly from todo
    EXPECT_EQ(llm_client_ptr->plan_requests[0], "Create hello world - Python script");
}

} // namespace mag