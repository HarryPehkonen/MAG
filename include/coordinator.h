#pragma once

#include "message.h"
#include "policy.h"
#include "todo_manager.h"
#include "bash_tool.h"
#include "llm_provider.h"
#include "interfaces/llm_client_interface.h"
#include "interfaces/file_client_interface.h"
#include <string>
#include <memory>
#include <atomic>

namespace mag {

class Coordinator {
public:
    // Execution control state
    enum class ExecutionState { STOPPED, RUNNING, PAUSED, CANCELLED };
    
    // Original constructors for backward compatibility
    Coordinator();
    Coordinator(const std::string& provider_override);
    
    // New constructor with dependency injection (for testing and flexibility)
    Coordinator(std::unique_ptr<ILLMClient> llm_client, 
                std::unique_ptr<IFileClient> file_client,
                PolicyChecker policy_checker = PolicyChecker(),
                TodoManager todo_manager = TodoManager());
    
    ~Coordinator();
    
    void run(const std::string& user_prompt);
    std::string run_with_conversation_history(const std::string& user_prompt, 
                                             const std::vector<ConversationMessage>& conversation_history);
    void set_provider(const std::string& provider_name);
    std::string get_current_provider() const { return current_provider_; }
    void set_chat_mode(bool enabled);
    void toggle_chat_mode();
    
    // Todo operations
    TodoManager& get_todo_manager() { return todo_manager_; }
    void execute_todos(); // Execute all pending todos
    void execute_next_todo(); // Execute next pending todo
    void execute_todos_until(int stop_id); // Execute todos until (not including) stop_id  
    void execute_todos_range(int start_id, int end_id); // Execute todos in range [start_id, end_id]
    void execute_single_todo(const TodoItem& todo);
    
    // Execution control methods
    void pause_execution();
    void resume_execution();
    void stop_execution();
    void cancel_execution();
    ExecutionState get_execution_state() const { return execution_state_; }
    
private:
    PolicyChecker policy_checker_;
    TodoManager todo_manager_;
    bool always_approve_ = false;
    bool chat_mode_ = true; // Default to chat mode
    
    ExecutionState execution_state_ = ExecutionState::STOPPED;
    std::atomic<bool> should_stop_execution_{false};
    std::atomic<bool> should_pause_execution_{false};
    
    // Interface-based communication (new design)
    std::unique_ptr<ILLMClient> llm_client_;
    std::unique_ptr<IFileClient> file_client_;
    
    // Legacy NNG communication (for backward compatibility)
    void* llm_socket_;
    void* file_socket_;
    void* bash_socket_;
    std::string current_provider_;
    
    // Initialization methods
    void initialize_sockets();
    void cleanup_sockets();
    void initialize_with_defaults();
    
    // Network communication methods
    WriteFileCommand request_plan_from_llm(const std::string& user_prompt);
    GenericCommand request_generic_plan_from_llm(const std::string& user_prompt);
    std::string request_chat_from_llm(const std::string& user_prompt);
    std::string request_chat_from_llm_with_history(const std::string& user_prompt,
                                                   const std::vector<ConversationMessage>& conversation_history);
    DryRunResult request_dry_run(const WriteFileCommand& command);
    ApplyResult request_apply(const WriteFileCommand& command);
    
    // Bash command communication
    CommandResult request_bash_execution(const BashCommand& command);
    
    // Todo execution methods
    bool should_execute_as_bash_command(const std::string& prompt);
    void execute_todo_as_bash_command(const TodoItem& todo);
    void execute_todo_as_file_operation(const TodoItem& todo);
    void execute_generic_command(const GenericCommand& command);
    
    // Helper methods
    std::string extract_bash_command_from_prompt(const std::string& prompt);
    void display_bash_result(const CommandResult& result);
    
    bool get_user_confirmation(const DryRunResult& dry_run_result);
    void display_result(const ApplyResult& result);
    std::string parse_and_execute_todo_operations(const std::string& llm_response);
};

} // namespace mag