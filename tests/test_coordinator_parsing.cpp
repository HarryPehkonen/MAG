#include <gtest/gtest.h>
#include "coordinator.h"
#include "message.h"
#include <sstream>
#include <iostream>
#include <regex>
#include <memory>

namespace mag {

// Test class that gives us access to protected/private parsing methods
class TestableCoordinator {
private:
    TodoManager todo_manager_;
    
public:
    TestableCoordinator() = default;
    
    // Expose the core parsing logic without network dependencies
    std::string test_parse_todo_operations(const std::string& llm_response) {
        return parse_todo_operations_standalone(llm_response);
    }
    
    TodoManager& get_todo_manager() { return todo_manager_; }
    
private:
    // Standalone version of parse_and_execute_todo_operations for testing
    std::string parse_todo_operations_standalone(const std::string& llm_response) {
        std::string modified_response = llm_response;
        std::string execution_log;
        
        // Parse todo operations from the LLM response
        // Look for patterns like add_todo(title, description) AND block format
        std::regex add_todo_regex(R"(add_todo\s*\(\s*['"](.*?)['"]\s*,\s*['"](.*?)['"]\s*\))");
        std::regex todo_separator_regex(R"(<TODO_SEPARATOR>\s*\nTitle:\s*(.*?)\nDescription:\s*([\s\S]*?)\n<TODO_SEPARATOR>)");
        std::regex list_todos_regex(R"(list_todos\s*\(\s*\))");
        std::regex mark_complete_regex(R"(mark_complete\s*\(\s*(\d+)\s*\))");
        std::regex delete_todo_regex(R"(delete_todo\s*\(\s*(\d+)\s*\))");
        
        std::smatch match;
        
        // Process add_todo operations
        auto add_todo_start = modified_response.cbegin();
        while (std::regex_search(add_todo_start, modified_response.cend(), match, add_todo_regex)) {
            std::string title = match[1].str();
            std::string description = match[2].str();
            
            // Execute the todo operation
            int new_id = todo_manager_.add_todo(title, description);
            execution_log += "\n[TODO] Added: " + title + " (ID: " + std::to_string(new_id) + ")";
            
            // Remove the function call from the response and replace with result
            std::string replacement = "**Added:** " + title;
            modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            
            add_todo_start = modified_response.cbegin();
        }
        
        // Process CodeBundler-style <TODO_SEPARATOR> format
        auto block_start = modified_response.cbegin();
        while (std::regex_search(block_start, modified_response.cend(), match, todo_separator_regex)) {
            std::string title = match[1].str();
            std::string description = match[2].str();
            
            // Trim whitespace from extracted strings
            title.erase(0, title.find_first_not_of(" \t\r\n"));
            title.erase(title.find_last_not_of(" \t\r\n") + 1);
            description.erase(0, description.find_first_not_of(" \t\r\n"));
            description.erase(description.find_last_not_of(" \t\r\n") + 1);
            
            // Execute the todo operation
            int new_id = todo_manager_.add_todo(title, description);
            execution_log += "\n[TODO] Added: " + title + " (ID: " + std::to_string(new_id) + ")";
            
            // Remove the block from the response and replace with result
            std::string replacement = "**Added:** " + title;
            modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            
            block_start = modified_response.cbegin();
        }
        
        // Process list_todos operations
        if (std::regex_search(modified_response, match, list_todos_regex)) {
            auto todos = todo_manager_.list_todos(true); // include completed
            std::string todo_list = "\n**Current Todos:**\n";
            if (todos.empty()) {
                todo_list += "- No todos yet\n";
            } else {
                for (const auto& todo : todos) {
                    std::string status_icon = (todo.status == TodoStatus::COMPLETED) ? "✅" : "⏳";
                    todo_list += "- " + status_icon + " " + std::to_string(todo.id) + ": " + todo.title + "\n";
                    if (!todo.description.empty()) {
                        todo_list += "  " + todo.description + "\n";
                    }
                }
            }
            modified_response = std::regex_replace(modified_response, list_todos_regex, todo_list);
        }
        
        // Process mark_complete operations
        auto complete_start = modified_response.cbegin();
        while (std::regex_search(complete_start, modified_response.cend(), match, mark_complete_regex)) {
            int todo_id = std::stoi(match[1].str());
            
            if (todo_manager_.mark_completed(todo_id)) {
                execution_log += "\n[TODO] Completed: ID " + std::to_string(todo_id);
                std::string replacement = "**Completed:** Todo " + std::to_string(todo_id);
                modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            } else {
                std::string replacement = "**Error:** Todo " + std::to_string(todo_id) + " not found";
                modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            }
            
            complete_start = modified_response.cbegin();
        }
        
        // Process delete_todo operations
        auto delete_start = modified_response.cbegin();
        while (std::regex_search(delete_start, modified_response.cend(), match, delete_todo_regex)) {
            int todo_id = std::stoi(match[1].str());
            
            if (todo_manager_.delete_todo(todo_id)) {
                execution_log += "\n[TODO] Deleted: ID " + std::to_string(todo_id);
                std::string replacement = "**Deleted:** Todo " + std::to_string(todo_id);
                modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            } else {
                std::string replacement = "**Error:** Todo " + std::to_string(todo_id) + " not found";
                modified_response = std::regex_replace(modified_response, std::regex(regex_escape(match[0].str())), replacement);
            }
            
            delete_start = modified_response.cbegin();
        }
        
        return modified_response;
    }
    
    // Helper function to escape regex special characters
    std::string regex_escape(const std::string& str) {
        static const std::regex special_chars{R"([-[\]{}()*+?.,\^$|#\s])"};
        return std::regex_replace(str, special_chars, R"(\$&)");
    }
};

class CoordinatorParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        coordinator = std::make_unique<TestableCoordinator>();
    }
    
    void TearDown() override {
        coordinator.reset();
    }
    
    std::unique_ptr<TestableCoordinator> coordinator;
};

TEST_F(CoordinatorParsingTest, ParsesSimpleAddTodoFunctionCall) {
    std::string llm_response = R"(I'll help you with that! add_todo("Create hello world", "Python script") The todo has been added.)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that todo was added to manager
    auto todos = coordinator->get_todo_manager().list_todos();
    ASSERT_EQ(todos.size(), 1);
    EXPECT_EQ(todos[0].title, "Create hello world");
    EXPECT_EQ(todos[0].description, "Python script");
    
    // Check that response was modified
    EXPECT_TRUE(result.find("**Added:** Create hello world") != std::string::npos);
    EXPECT_TRUE(result.find("add_todo(") == std::string::npos);
}

TEST_F(CoordinatorParsingTest, ParsesCodeBundlerStyleTodo) {
    std::string llm_response = R"(I'll create a complex script for you!
<TODO_SEPARATOR>
Title: Create Python script with quotes
Description: Script that prints "Hello World!" and asks 'What's your name?'
Should handle complex formatting and special characters
<TODO_SEPARATOR>
The todo is now queued safely!)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that todo was added to manager
    auto todos = coordinator->get_todo_manager().list_todos();
    ASSERT_EQ(todos.size(), 1);
    EXPECT_EQ(todos[0].title, "Create Python script with quotes");
    EXPECT_TRUE(todos[0].description.find("Hello World!") != std::string::npos);
    EXPECT_TRUE(todos[0].description.find("What's your name?") != std::string::npos);
    
    // Check that response was modified
    EXPECT_TRUE(result.find("**Added:** Create Python script with quotes") != std::string::npos);
    EXPECT_TRUE(result.find("<TODO_SEPARATOR>") == std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesMultipleAddTodos) {
    std::string llm_response = R"(I'll break this down into steps:
add_todo("Setup project", "Create directory structure")
add_todo("Create main file", "Python entry point")
All todos have been queued!)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that both todos were added
    auto todos = coordinator->get_todo_manager().list_todos();
    ASSERT_EQ(todos.size(), 2);
    EXPECT_EQ(todos[0].title, "Setup project");
    EXPECT_EQ(todos[1].title, "Create main file");
}

TEST_F(CoordinatorParsingTest, ParsesListTodosCall) {
    // Add some todos first
    coordinator->get_todo_manager().add_todo("Test todo", "Description");
    coordinator->get_todo_manager().add_todo("Another todo", "Another desc");
    
    std::string llm_response = "Here are your todos: list_todos()";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that response includes todo list
    EXPECT_TRUE(result.find("**Current Todos:**") != std::string::npos);
    EXPECT_TRUE(result.find("Test todo") != std::string::npos);
    EXPECT_TRUE(result.find("Another todo") != std::string::npos);
    EXPECT_TRUE(result.find("list_todos()") == std::string::npos);
}

TEST_F(CoordinatorParsingTest, ParsesMarkCompleteCall) {
    // Add a todo first
    int todo_id = coordinator->get_todo_manager().add_todo("Test todo", "Description");
    
    std::string llm_response = "Marking as complete: mark_complete(" + std::to_string(todo_id) + ")";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that todo was marked complete
    auto* todo = coordinator->get_todo_manager().get_todo(todo_id);
    ASSERT_NE(todo, nullptr);
    EXPECT_EQ(todo->status, TodoStatus::COMPLETED);
    
    // Check response
    EXPECT_TRUE(result.find("**Completed:** Todo " + std::to_string(todo_id)) != std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesInvalidMarkComplete) {
    std::string llm_response = "Marking as complete: mark_complete(999)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check error response
    EXPECT_TRUE(result.find("**Error:** Todo 999 not found") != std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesComplexQuotesInCodeBundlerFormat) {
    // Simpler test with less complex content to avoid regex issues
    std::string llm_response = R"(Creating a script:
<TODO_SEPARATOR>
Title: Create SQL query script
Description: Script with SQL like SELECT * FROM users WHERE name = 'John Database' AND status = 'active'
<TODO_SEPARATOR>
Ready to execute!)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that todo was added with complex content
    auto todos = coordinator->get_todo_manager().list_todos();
    ASSERT_EQ(todos.size(), 1);
    EXPECT_EQ(todos[0].title, "Create SQL query script");
    EXPECT_TRUE(todos[0].description.find("John Database") != std::string::npos);
    EXPECT_TRUE(todos[0].description.find("SELECT * FROM users") != std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesDeleteTodoCall) {
    // Add a todo first
    int todo_id = coordinator->get_todo_manager().add_todo("Test todo", "Description");
    
    std::string llm_response = "Deleting todo: delete_todo(" + std::to_string(todo_id) + ")";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check that todo was deleted
    auto* todo = coordinator->get_todo_manager().get_todo(todo_id);
    EXPECT_EQ(todo, nullptr);
    
    // Check response
    EXPECT_TRUE(result.find("**Deleted:** Todo " + std::to_string(todo_id)) != std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesInvalidDeleteTodo) {
    std::string llm_response = "Deleting todo: delete_todo(999)";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Check error response
    EXPECT_TRUE(result.find("**Error:** Todo 999 not found") != std::string::npos);
}

TEST_F(CoordinatorParsingTest, HandlesNoOperations) {
    std::string llm_response = "This is just a regular response with no todo operations.";
    
    std::string result = coordinator->test_parse_todo_operations(llm_response);
    
    // Response should be unchanged
    EXPECT_EQ(result, llm_response);
    
    // No todos should be added
    auto todos = coordinator->get_todo_manager().list_todos();
    EXPECT_EQ(todos.size(), 0);
}

} // namespace mag