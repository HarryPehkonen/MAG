#include "bash_tool.h"
#include "utils.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cstdio>
#include <memory>
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace mag {

// CommandResult helper methods
std::string CommandResult::get_combined_output() const {
    std::string combined = stdout_output;
    if (!stderr_output.empty()) {
        if (!combined.empty()) combined += "\n";
        combined += "[STDERR]: " + stderr_output;
    }
    return combined;
}

std::string CommandResult::to_string() const {
    std::ostringstream oss;
    oss << "Command: " << command << "\n"
        << "Exit Code: " << exit_code << "\n"
        << "Working Directory: " << working_directory << "\n"
        << "PWD After: " << pwd_after_execution << "\n"
        << "Duration: " << execution_duration.count() << "ms\n"
        << "Success: " << (success ? "true" : "false") << "\n";
    
    if (!stdout_output.empty()) {
        oss << "Output:\n" << stdout_output << "\n";
    }
    
    if (!stderr_output.empty()) {
        oss << "Error Output:\n" << stderr_output << "\n";
    }
    
    if (!error_message.empty()) {
        oss << "Error: " << error_message << "\n";
    }
    
    return oss.str();
}

bool CommandResult::has_output() const {
    return !stdout_output.empty() || !stderr_output.empty();
}

// BashTool implementation
BashTool::BashTool() {
    // Initialize with sensible defaults
    capture_context_ = true;
    default_timeout_ms_ = 30000;
}

CommandResult BashTool::execute_command(const std::string& command, 
                                       const std::string& working_directory,
                                       int timeout_ms) {
    CommandResult result;
    result.command = command;
    result.start_time = std::chrono::system_clock::now();
    
    // Security check
    if (!is_command_allowed(command)) {
        result.success = false;
        result.exit_code = -1;
        result.error_message = "Command blocked by security policy: " + command;
        result.end_time = std::chrono::system_clock::now();
        result.execution_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.end_time - result.start_time);
        return result;
    }
    
    // Set working directory
    std::string work_dir = working_directory.empty() ? get_current_directory() : working_directory;
    result.working_directory = work_dir;
    
    try {
        // Execute command based on platform
#ifdef _WIN32
        result = execute_windows_command(command, work_dir, timeout_ms);
#else
        result = execute_unix_command(command, work_dir, timeout_ms);
#endif
        
        // Capture execution context if enabled
        if (capture_context_) {
            capture_execution_context(result);
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.exit_code = -1;
        result.error_message = e.what();
    }
    
    result.end_time = std::chrono::system_clock::now();
    result.execution_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        result.end_time - result.start_time);
    
    return result;
}

CommandResult BashTool::execute_with_context(const std::string& command,
                                            const std::string& working_directory) {
    // Temporarily enable context capture
    bool original_capture = capture_context_;
    capture_context_ = true;
    
    CommandResult result = execute_command(command, working_directory);
    
    // Restore original setting
    capture_context_ = original_capture;
    
    return result;
}

std::string BashTool::get_current_directory() const {
    return Utils::get_current_working_directory();
}

bool BashTool::is_command_allowed(const std::string& command) const {
    // Check against blocked commands
    auto blocked = get_blocked_commands();
    std::string cmd_lower = command;
    std::transform(cmd_lower.begin(), cmd_lower.end(), cmd_lower.begin(), ::tolower);
    
    for (const auto& blocked_cmd : blocked) {
        if (cmd_lower.find(blocked_cmd) == 0 || 
            cmd_lower.find(" " + blocked_cmd) != std::string::npos) {
            return false;
        }
    }
    
    // Check for dangerous patterns
    if (contains_dangerous_pattern(command)) {
        return false;
    }
    
    return true;
}

std::vector<std::string> BashTool::get_blocked_commands() const {
    return {
        "rm -rf /",
        "sudo rm",
        "format",
        "fdisk",
        "mkfs",
        "dd if=/dev/zero",
        ":(){ :|:& };:",  // Fork bomb
        "chmod 000",
        "chown root",
        "passwd",
        "su -",
        "sudo su",
        "reboot",
        "shutdown",
        "halt",
        "poweroff",
        "init 0",
        "init 6"
    };
}

std::vector<std::string> BashTool::get_dangerous_patterns() const {
    return {
        R"(>\s*/dev/)",     // Redirecting to device files
        R"(/dev/sd[a-z])",  // Direct disk access
        R"(rm\s+.*-rf)",    // Recursive force remove
        R"(\|.*rm)",        // Piped to rm
        R"(;\s*rm)",        // Chained with rm
        R"(&&.*rm)",        // AND chained with rm
        R"(\$\([^)]*rm)",   // Command substitution with rm
    };
}

bool BashTool::contains_dangerous_pattern(const std::string& command) const {
    auto patterns = get_dangerous_patterns();
    
    for (const auto& pattern : patterns) {
        try {
            std::regex danger_regex(pattern, std::regex_constants::icase);
            if (std::regex_search(command, danger_regex)) {
                return true;
            }
        } catch (const std::regex_error& e) {
            // If regex fails, err on the side of caution
            std::cerr << "Regex error in security check: " << e.what() << std::endl;
            return true;
        }
    }
    
    return false;
}

CommandResult BashTool::execute_unix_command(const std::string& command, 
                                            const std::string& working_directory,
                                            int timeout_ms) {
    CommandResult result;
    result.command = command;
    result.working_directory = working_directory;
    
    // Prepare command with working directory change and pwd capture
    std::string full_command = "cd \"" + working_directory + "\" && " + command;
    
    // Append pwd capture to get final working directory
    if (capture_context_) {
        full_command += " ; echo \"__PWD_MARKER__$(pwd)\"";
    }
    
    // Use popen to capture output
    std::string cmd_with_stderr = full_command + " 2>&1";
    
    FILE* pipe = popen(cmd_with_stderr.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }
    
    // Read output
    char buffer[128];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    // Get exit code
    int status = pclose(pipe);
    result.exit_code = WEXITSTATUS(status);
    result.success = (result.exit_code == 0);
    
    // Extract pwd if captured
    if (capture_context_) {
        std::string pwd_marker = "__PWD_MARKER__";
        size_t pwd_pos = output.find(pwd_marker);
        if (pwd_pos != std::string::npos) {
            size_t pwd_start = pwd_pos + pwd_marker.length();
            size_t pwd_end = output.find('\n', pwd_start);
            if (pwd_end == std::string::npos) pwd_end = output.length();
            
            result.pwd_after_execution = output.substr(pwd_start, pwd_end - pwd_start);
            
            // Remove the pwd marker line from output
            size_t line_start = output.rfind('\n', pwd_pos);
            if (line_start == std::string::npos) line_start = 0;
            else line_start++; // Skip the newline
            
            output.erase(line_start, pwd_end - line_start + (pwd_end < output.length() ? 1 : 0));
        }
    }
    
    result.stdout_output = output;
    
    return result;
}

CommandResult BashTool::execute_windows_command(const std::string& command,
                                               const std::string& working_directory,
                                               int timeout_ms) {
    CommandResult result;
    result.command = command;
    result.working_directory = working_directory;
    
    // For Windows, use cmd /c with directory change and pwd capture
    std::string full_command = "cd /d \"" + working_directory + "\" && " + command;
    
    // Append pwd capture to get final working directory
    if (capture_context_) {
        full_command += " && echo __PWD_MARKER__%cd%";
    }
    
    std::string cmd_with_stderr = "cmd /c \"" + full_command + "\" 2>&1";
    
    FILE* pipe = popen(cmd_with_stderr.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }
    
    // Read output
    char buffer[128];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    // Get exit code
    int status = pclose(pipe);
    result.exit_code = status;
    result.success = (result.exit_code == 0);
    
    // Extract pwd if captured
    if (capture_context_) {
        std::string pwd_marker = "__PWD_MARKER__";
        size_t pwd_pos = output.find(pwd_marker);
        if (pwd_pos != std::string::npos) {
            size_t pwd_start = pwd_pos + pwd_marker.length();
            size_t pwd_end = output.find('\n', pwd_start);
            if (pwd_end == std::string::npos) pwd_end = output.length();
            
            result.pwd_after_execution = output.substr(pwd_start, pwd_end - pwd_start);
            
            // Remove the pwd marker line from output
            size_t line_start = output.rfind('\n', pwd_pos);
            if (line_start == std::string::npos) line_start = 0;
            else line_start++; // Skip the newline
            
            output.erase(line_start, pwd_end - line_start + (pwd_end < output.length() ? 1 : 0));
        }
    }
    
    result.stdout_output = output;
    
    return result;
}

std::string BashTool::sanitize_command(const std::string& command) const {
    // Remove potentially dangerous characters
    std::string sanitized = command;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    return sanitized;
}

std::string BashTool::get_shell_command() const {
#ifdef _WIN32
    return "cmd /c";
#else
    return "/bin/bash -c";
#endif
}

void BashTool::capture_execution_context(CommandResult& result) {
    // Context capture is now handled directly in the command execution methods
    // This method is kept for compatibility but is no longer needed for pwd capture
    
    // If pwd_after_execution is still empty (command execution didn't capture it),
    // fall back to the original working directory
    if (result.pwd_after_execution.empty()) {
        result.pwd_after_execution = result.working_directory;
    }
}

} // namespace mag