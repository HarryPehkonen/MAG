#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace mag {

/**
 * @brief Result of executing a bash command
 * 
 * Contains comprehensive information about command execution including
 * output, exit code, execution context, and timing information.
 */
struct CommandResult {
    std::string command;              // The command that was executed
    int exit_code;                    // Exit code from command execution
    std::string stdout_output;        // Standard output from the command
    std::string stderr_output;        // Standard error from the command
    std::string working_directory;    // Working directory where command was executed
    std::string pwd_after_execution;  // Working directory after command execution
    bool success;                     // Whether the command succeeded (exit_code == 0)
    std::string error_message;        // Error message if execution failed
    
    // Timing information
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::milliseconds execution_duration;
    
    // Helper methods
    std::string get_combined_output() const;
    std::string to_string() const;
    bool has_output() const;
};

/**
 * @brief Tool for executing bash commands with comprehensive result capture
 * 
 * Provides safe, controlled execution of bash commands with:
 * - Comprehensive output capture (stdout, stderr)
 * - Working directory tracking
 * - Execution timing
 * - Security policies and command filtering
 * - Cross-platform compatibility
 */
class BashTool {
public:
    BashTool();
    ~BashTool() = default;
    
    /**
     * @brief Execute a bash command and capture results
     * @param command The command to execute
     * @param working_directory Optional working directory (uses current if empty)
     * @param timeout_ms Timeout in milliseconds (default 30 seconds)
     * @return CommandResult with execution details
     * @throws std::runtime_error if command execution fails
     */
    CommandResult execute_command(const std::string& command, 
                                 const std::string& working_directory = "",
                                 int timeout_ms = 30000);
    
    /**
     * @brief Execute command and capture pwd context automatically
     * @param command The command to execute
     * @param working_directory Optional working directory
     * @return CommandResult with pwd captured after execution
     */
    CommandResult execute_with_context(const std::string& command,
                                      const std::string& working_directory = "");
    
    /**
     * @brief Get current working directory
     * @return Current working directory path
     */
    std::string get_current_directory() const;
    
    /**
     * @brief Check if a command is allowed by security policies
     * @param command The command to check
     * @return true if command is allowed, false otherwise
     */
    bool is_command_allowed(const std::string& command) const;
    
    /**
     * @brief Set whether to capture context (pwd) after each command
     * @param capture_context If true, automatically capture pwd after execution
     */
    void set_capture_context(bool capture_context) { capture_context_ = capture_context; }
    
    /**
     * @brief Set command timeout in milliseconds
     * @param timeout_ms Default timeout for command execution
     */
    void set_default_timeout(int timeout_ms) { default_timeout_ms_ = timeout_ms; }

private:
    bool capture_context_ = true;        // Whether to auto-capture pwd after execution
    int default_timeout_ms_ = 30000;     // Default timeout (30 seconds)
    
    // Security and policy methods
    std::vector<std::string> get_blocked_commands() const;
    std::vector<std::string> get_dangerous_patterns() const;
    bool contains_dangerous_pattern(const std::string& command) const;
    
    // Platform-specific execution methods
    CommandResult execute_unix_command(const std::string& command, 
                                      const std::string& working_directory,
                                      int timeout_ms);
    CommandResult execute_windows_command(const std::string& command,
                                         const std::string& working_directory, 
                                         int timeout_ms);
    
    // Helper methods
    std::string sanitize_command(const std::string& command) const;
    std::string get_shell_command() const;
    void capture_execution_context(CommandResult& result);
};

} // namespace mag