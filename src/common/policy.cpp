#include "policy.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace mag {

PolicyChecker::PolicyChecker() {
    // Load or create policy configuration
    settings_ = PolicyConfig::load_or_create();
    if (!settings_) {
        // This should not happen due to load_or_create's error handling
        std::cerr << "CRITICAL: Failed to initialize policy settings" << std::endl;
        std::exit(1);
    }
}

bool PolicyChecker::is_allowed(const std::string& tool, Operation operation, const std::string& path) const {
    // First check if path is within current working directory
    if (!is_within_cwd(path)) {
        return false;
    }
    
    // Check if file extension is blocked
    if (is_extension_blocked(path)) {
        return false;
    }
    
    // Use PolicySettings method for tool-specific operation checking
    return settings_->is_operation_allowed(tool, operation, path);
}

bool PolicyChecker::is_allowed(const std::string& path) const {
    // Legacy method - default to file_tool READ operation for backward compatibility
    return is_allowed("file_tool", Operation::READ, path);
}

bool PolicyChecker::is_extension_blocked(const std::string& path) const {
    std::filesystem::path file_path(path);
    std::string extension = file_path.extension().string();
    
    if (extension.empty()) {
        return false; // No extension, not blocked
    }
    
    return std::find(settings_->global.blocked_extensions.begin(), 
                     settings_->global.blocked_extensions.end(), 
                     extension) != settings_->global.blocked_extensions.end();
}

bool PolicyChecker::is_file_size_allowed(size_t size_bytes) const {
    size_t max_bytes = settings_->global.max_file_size_mb * 1024 * 1024;
    return size_bytes <= max_bytes;
}

bool PolicyChecker::update_settings(const PolicySettings& new_settings, std::string& error_message) {
    // Validate the new settings
    if (!new_settings.validate(error_message)) {
        return false;
    }
    
    // Save to file
    if (!PolicyConfig::save(new_settings, error_message)) {
        return false;
    }
    
    // Update in-memory settings
    settings_ = std::make_unique<PolicySettings>(new_settings);
    return true;
}

bool PolicyChecker::is_within_cwd(const std::string& path) const {
    std::string real_path = Utils::get_real_path(path);
    std::string safe_cwd = Utils::get_real_path(Utils::get_current_working_directory());
    
    // Ensure the path starts with the current working directory
    return real_path.find(safe_cwd) == 0;
}

bool PolicyChecker::has_allowed_prefix(const std::string& tool, Operation operation, const std::string& path) const {
    // Use PolicySettings method for tool-specific operation checking
    return settings_->is_operation_allowed(tool, operation, path);
}

bool PolicyChecker::has_allowed_prefix(const std::string& path) const {
    // Legacy method - default to file_tool READ operation for backward compatibility
    return has_allowed_prefix("file_tool", Operation::READ, path);
}

std::vector<std::string> PolicyChecker::get_allowed_directories(const std::string& tool, const std::string& operation) const {
    // Convert string operation to enum
    Operation op;
    if (operation == "create") {
        op = Operation::CREATE;
    } else if (operation == "read") {
        op = Operation::READ;
    } else if (operation == "update") {
        op = Operation::UPDATE;
    } else if (operation == "delete") {
        op = Operation::DELETE;
    } else {
        return {}; // Unknown operation
    }
    
    // Get tool policy from settings
    auto tool_policies = settings_->tools;
    auto tool_it = tool_policies.find(tool);
    if (tool_it == tool_policies.end()) {
        return {}; // Tool not found
    }
    
    const ToolPolicy& tool_policy = tool_it->second;
    
    // Get allowed directories for the specific operation
    switch (op) {
        case Operation::CREATE:
            return tool_policy.create.allowed_directories;
        case Operation::READ:
            return tool_policy.read.allowed_directories;
        case Operation::UPDATE:
            return tool_policy.update.allowed_directories;
        case Operation::DELETE:
            return tool_policy.delete_op.allowed_directories;
        default:
            return {};
    }
}

bool PolicyChecker::is_bash_command_allowed(const std::string& command) const {
    // First check if command is explicitly blocked
    if (is_bash_command_blocked(command)) {
        return false;
    }
    
    // Get bash_tool policy
    auto bash_tool_it = settings_->tools.find("bash_tool");
    if (bash_tool_it == settings_->tools.end()) {
        return false; // No bash_tool policy found
    }
    
    const auto& bash_policy = bash_tool_it->second.create;
    
    // If no allowed commands specified, allow anything not explicitly blocked
    if (bash_policy.allowed_commands.empty()) {
        return true;
    }
    
    // Extract the base command (first word)
    std::string base_command = command;
    size_t space_pos = command.find(' ');
    if (space_pos != std::string::npos) {
        base_command = command.substr(0, space_pos);
    }
    
    // Check if base command is in allowed list
    return std::find(bash_policy.allowed_commands.begin(), 
                     bash_policy.allowed_commands.end(), 
                     base_command) != bash_policy.allowed_commands.end();
}

bool PolicyChecker::is_bash_command_blocked(const std::string& command) const {
    auto bash_tool_it = settings_->tools.find("bash_tool");
    if (bash_tool_it == settings_->tools.end()) {
        return true; // No bash_tool policy = blocked
    }
    
    const auto& bash_policy = bash_tool_it->second.create;
    
    // Check against blocked commands
    for (const auto& blocked_cmd : bash_policy.blocked_commands) {
        if (command.find(blocked_cmd) != std::string::npos) {
            return true; // Command contains blocked substring
        }
    }
    
    return false;
}

std::string PolicyChecker::get_bash_command_violation_reason(const std::string& command) const {
    if (is_bash_command_blocked(command)) {
        return "Command contains blocked operation";
    }
    
    if (!is_bash_command_allowed(command)) {
        return "Command not in allowed list";
    }
    
    return ""; // No violation
}

} // namespace mag