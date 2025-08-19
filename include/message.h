#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace mag {

// Forward declarations
struct WriteFileCommand;

/**
 * @brief Type of operation to perform
 */
enum class OperationType {
    FILE_WRITE,    // Create or write to a file
    BASH_COMMAND   // Execute a bash command
};

/**
 * @brief Execution context captured after operation completion
 */
struct ExecutionContext {
    std::string working_directory_before;   // Working directory before execution
    std::string working_directory_after;    // Working directory after execution (from pwd)
    std::string command_output;              // Output from the executed command/operation
    std::string command_stderr;              // Error output if any
    int exit_code = 0;                       // Exit code (0 for file operations)
    std::chrono::system_clock::time_point timestamp;  // When context was captured
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
    
    // Helper methods
    bool has_output() const;
    std::string get_combined_output() const;
    std::string to_summary_string() const;
};

struct WriteFileCommand {
    std::string command;
    std::string path;
    std::string content;
    bool request_execution = false;  // Whether LLM suggests executing after creation
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

/**
 * @brief Command structure for bash operations
 */
struct BashCommand {
    std::string command;
    std::string bash_command;
    std::string working_directory;  // Optional working directory override
    std::string description;        // Human-readable description
    bool request_execution = false; // Whether LLM suggests executing this command
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
    
    // Helper methods
    bool has_working_directory() const { return !working_directory.empty(); }
    std::string get_summary() const;
};

/**
 * @brief Generic command structure that can represent different operation types
 */
struct GenericCommand {
    OperationType type;
    std::string description;  // Human-readable description of the operation
    
    // For FILE_WRITE operations
    std::string file_path;
    std::string file_content;
    
    // For BASH_COMMAND operations
    std::string bash_command;
    std::string working_directory;  // Optional working directory for bash commands
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
    
    // Helper methods
    bool is_file_operation() const { return type == OperationType::FILE_WRITE; }
    bool is_bash_operation() const { return type == OperationType::BASH_COMMAND; }
    WriteFileCommand to_write_file_command() const;
    std::string get_operation_summary() const;
};

struct DryRunResult {
    std::string description;
    bool success;
    std::string error_message;
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
};

struct ApplyResult {
    std::string description;
    bool success;
    std::string error_message;
    ExecutionContext execution_context;  // Context captured after execution
    
    void to_json(nlohmann::json& j) const;
    void from_json(const nlohmann::json& j);
    
    // Helper methods
    std::string get_execution_summary() const;
    bool has_context_output() const;
};

class MessageHandler {
public:
    static std::string serialize_command(const WriteFileCommand& cmd);
    static WriteFileCommand deserialize_command(const std::string& json_str);
    
    static std::string serialize_dry_run_result(const DryRunResult& result);
    static DryRunResult deserialize_dry_run_result(const std::string& json_str);
    
    static std::string serialize_apply_result(const ApplyResult& result);
    static ApplyResult deserialize_apply_result(const std::string& json_str);
    
    static std::string serialize_execution_context(const ExecutionContext& context);
    static ExecutionContext deserialize_execution_context(const std::string& json_str);
    
    static std::string serialize_bash_command(const BashCommand& cmd);
    static BashCommand deserialize_bash_command(const std::string& json_str);
};

} // namespace mag