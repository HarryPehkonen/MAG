#include "policy_config.h"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace mag {

// Default policy settings with tool-specific CRUD operations
PolicySettings::PolicySettings() {
    // Initialize file_tool with sensible defaults
    ToolPolicy file_tool_policy;
    file_tool_policy.create = OperationPolicy({"src/", "tests/", "docs/"}, true);
    file_tool_policy.read = OperationPolicy({"src/", "tests/", "docs/"}, false);
    file_tool_policy.update = OperationPolicy({"src/", "tests/"}, true);
    file_tool_policy.delete_op = OperationPolicy({}, true); // Empty = disabled
    
    tools["file_tool"] = file_tool_policy;
    
    // Initialize todo_tool with permissive defaults (no file operations)
    ToolPolicy todo_tool_policy;
    todo_tool_policy.create = OperationPolicy({}, false);
    todo_tool_policy.read = OperationPolicy({}, false);
    todo_tool_policy.update = OperationPolicy({}, false);
    todo_tool_policy.delete_op = OperationPolicy({}, true);
    
    tools["todo_tool"] = todo_tool_policy;
    
    // Initialize bash_tool with security-focused defaults
    ToolPolicy bash_tool_policy;
    OperationPolicy bash_create({}, true);
    bash_create.allowed_commands = {"make", "cmake", "gcc", "g++", "npm", "cargo", "python", "python3", "pip", "ls", "pwd", "find", "grep", "cat", "head", "tail", "wc", "sort", "uniq", "awk", "sed", "git"};
    bash_create.blocked_commands = {"rm", "rmdir", "dd", "mkfs", "format", "fdisk", "mount", "umount", "chmod 777", "chown", "su", "sudo", "passwd", "systemctl", "shutdown", "reboot", "kill -9", "curl", "wget", "nc"};
    bash_tool_policy.create = bash_create;
    bash_tool_policy.read = OperationPolicy({}, false);
    bash_tool_policy.update = OperationPolicy({}, true);
    bash_tool_policy.delete_op = OperationPolicy({}, true);
    
    tools["bash_tool"] = bash_tool_policy;
}

bool PolicySettings::validate(std::string& error_message) const {
    // Validate global settings
    for (const auto& ext : global.blocked_extensions) {
        if (ext.empty()) {
            error_message = "Empty extension in global.blocked_extensions";
            return false;
        }
        if (ext[0] != '.') {
            error_message = "Extension '" + ext + "' must start with '.' in global.blocked_extensions";
            return false;
        }
    }
    
    if (global.max_file_size_mb == 0 || global.max_file_size_mb > 1000) {
        error_message = "global.max_file_size_mb must be between 1 and 1000, got " + std::to_string(global.max_file_size_mb);
        return false;
    }
    
    // Validate tool policies
    for (const auto& [tool_name, tool_policy] : tools) {
        if (tool_name.empty()) {
            error_message = "Empty tool name in tools";
            return false;
        }
        
        // Validate each operation's allowed directories
        std::vector<std::pair<std::string, const OperationPolicy*>> ops = {
            {"create", &tool_policy.create},
            {"read", &tool_policy.read},
            {"update", &tool_policy.update},
            {"delete", &tool_policy.delete_op}
        };
        
        for (const auto& [op_name, op_policy] : ops) {
            for (const auto& dir : op_policy->allowed_directories) {
                if (dir.empty()) {
                    continue; // Empty string means "any directory" for some operations
                }
                if (dir.back() != '/') {
                    error_message = "Directory '" + dir + "' in " + tool_name + "." + op_name + " must end with '/'";
                    return false;
                }
                if (dir.find("..") != std::string::npos) {
                    error_message = "Directory '" + dir + "' in " + tool_name + "." + op_name + " contains invalid path traversal sequence '..'";
                    return false;
                }
            }
        }
    }
    
    return true;
}

const OperationPolicy* PolicySettings::get_operation_policy(const std::string& tool, Operation op) const {
    auto tool_it = tools.find(tool);
    if (tool_it == tools.end()) {
        return nullptr;
    }
    
    const ToolPolicy& tool_policy = tool_it->second;
    switch (op) {
        case Operation::CREATE: return &tool_policy.create;
        case Operation::READ: return &tool_policy.read;
        case Operation::UPDATE: return &tool_policy.update;
        case Operation::DELETE: return &tool_policy.delete_op;
    }
    return nullptr;
}

bool PolicySettings::is_operation_allowed(const std::string& tool, Operation op, const std::string& path) const {
    const OperationPolicy* policy = get_operation_policy(tool, op);
    if (!policy) {
        return false; // Tool or operation not found
    }
    
    // If no directories specified, operation is disabled
    if (policy->allowed_directories.empty()) {
        return false;
    }
    
    // Check if path matches any allowed directory
    for (const auto& allowed_dir : policy->allowed_directories) {
        if (allowed_dir.empty()) {
            return true; // Empty string means any directory allowed
        }
        if (path.find(allowed_dir) == 0) {
            return true; // Path starts with allowed directory
        }
    }
    
    return false;
}

std::unique_ptr<PolicySettings> PolicyConfig::load_or_create() {
    std::string error_message;
    std::string policy_file = get_policy_file_path();
    
    // Check if policy file exists
    if (!std::filesystem::exists(policy_file)) {
        std::cout << "Creating default policy configuration at " << policy_file << std::endl;
        
        // Create .mag directory and default config
        if (!ensure_mag_directory_exists(error_message)) {
            std::cerr << "ERROR: Failed to create .mag directory: " << error_message << std::endl;
            std::exit(2);
        }
        
        if (!create_default_config(error_message)) {
            std::cerr << "ERROR: Failed to create default policy.json: " << error_message << std::endl;
            std::exit(2);
        }
    }
    
    // Parse the policy file
    auto settings = parse_config(policy_file, error_message);
    if (!settings) {
        std::cerr << "ERROR: Failed to parse " << policy_file << std::endl;
        std::cerr << "├─ Issue: " << error_message << std::endl;
        std::cerr << "└─ Fix: Edit the file or delete it to regenerate defaults" << std::endl;
        std::exit(1);
    }
    
    return settings;
}

bool PolicyConfig::save(const PolicySettings& settings, std::string& error_message) {
    // Validate settings before saving
    if (!settings.validate(error_message)) {
        return false;
    }
    
    std::string policy_file = get_policy_file_path();
    
    try {
        nlohmann::json json = settings_to_json(settings);
        
        std::ofstream file(policy_file);
        if (!file.is_open()) {
            error_message = "Could not open " + policy_file + " for writing";
            return false;
        }
        
        file << json.dump(2) << std::endl;
        file.close();
        
        if (file.fail()) {
            error_message = "Failed to write to " + policy_file;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        error_message = "JSON serialization error: " + std::string(e.what());
        return false;
    }
}

std::string PolicyConfig::get_mag_directory() {
    return Utils::get_current_working_directory() + "/.mag";
}

std::string PolicyConfig::get_policy_file_path() {
    return get_mag_directory() + "/policy.json";
}

bool PolicyConfig::ensure_mag_directory_exists(std::string& error_message) {
    std::string mag_dir = get_mag_directory();
    
    try {
        if (!std::filesystem::exists(mag_dir)) {
            std::filesystem::create_directories(mag_dir);
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        error_message = "Filesystem error creating " + mag_dir + ": " + e.what();
        return false;
    }
}

bool PolicyConfig::create_default_config(std::string& error_message) {
    PolicySettings default_settings;
    return save(default_settings, error_message);
}

std::unique_ptr<PolicySettings> PolicyConfig::parse_config(const std::string& file_path, std::string& error_message) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            error_message = "Could not open file: " + file_path;
            return nullptr;
        }
        
        nlohmann::json json;
        file >> json;
        
        // Validate JSON schema
        if (!validate_json_schema(json, error_message)) {
            return nullptr;
        }
        
        // Convert to PolicySettings
        auto settings = json_to_settings(json);
        if (!settings) {
            error_message = "Failed to convert JSON to PolicySettings";
            return nullptr;
        }
        
        // Validate the settings
        std::string validation_error;
        if (!settings->validate(validation_error)) {
            error_message = "Policy validation failed: " + validation_error;
            return nullptr;
        }
        
        return settings;
        
    } catch (const nlohmann::json::parse_error& e) {
        error_message = "JSON parse error at byte " + std::to_string(e.byte) + ": " + e.what();
        return nullptr;
    } catch (const std::exception& e) {
        error_message = "Unexpected error: " + std::string(e.what());
        return nullptr;
    }
}

bool PolicyConfig::validate_json_schema(const nlohmann::json& json, std::string& error_message) {
    // Check version field
    if (!json.contains("version") || !json["version"].is_string()) {
        error_message = "Missing or invalid 'version' field (must be string)";
        return false;
    }
    
    // Check global section
    if (!json.contains("global") || !json["global"].is_object()) {
        error_message = "Missing or invalid 'global' field (must be object)";
        return false;
    }
    
    const auto& global = json["global"];
    if (!global.contains("blocked_extensions") || !global["blocked_extensions"].is_array()) {
        error_message = "Missing or invalid 'global.blocked_extensions' field (must be array)";
        return false;
    }
    
    if (!global.contains("max_file_size_mb") || !global["max_file_size_mb"].is_number_unsigned()) {
        error_message = "Missing or invalid 'global.max_file_size_mb' field (must be positive integer)";
        return false;
    }
    
    if (!global.contains("auto_backup") || !global["auto_backup"].is_boolean()) {
        error_message = "Missing or invalid 'global.auto_backup' field (must be boolean)";
        return false;
    }
    
    // Check tools section
    if (!json.contains("tools") || !json["tools"].is_object()) {
        error_message = "Missing or invalid 'tools' field (must be object)";
        return false;
    }
    
    // Validate each tool
    for (const auto& [tool_name, tool_data] : json["tools"].items()) {
        if (!tool_data.is_object()) {
            error_message = "Tool '" + tool_name + "' must be an object";
            return false;
        }
        
        // Check required operations
        std::vector<std::string> operations = {"create", "read", "update", "delete"};
        for (const std::string& op : operations) {
            if (!tool_data.contains(op) || !tool_data[op].is_object()) {
                error_message = "Missing or invalid '" + tool_name + "." + op + "' field (must be object)";
                return false;
            }
            
            const auto& op_data = tool_data[op];
            if (!op_data.contains("allowed_directories") || !op_data["allowed_directories"].is_array()) {
                error_message = "Missing or invalid '" + tool_name + "." + op + ".allowed_directories' field (must be array)";
                return false;
            }
            
            if (!op_data.contains("confirmation_required") || !op_data["confirmation_required"].is_boolean()) {
                error_message = "Missing or invalid '" + tool_name + "." + op + ".confirmation_required' field (must be boolean)";
                return false;
            }
        }
    }
    
    return true;
}

nlohmann::json PolicyConfig::settings_to_json(const PolicySettings& settings) {
    nlohmann::json json;
    json["version"] = "1.0";
    
    // Global settings
    json["global"] = {
        {"blocked_extensions", settings.global.blocked_extensions},
        {"max_file_size_mb", settings.global.max_file_size_mb},
        {"auto_backup", settings.global.auto_backup}
    };
    
    // Tool settings
    json["tools"] = nlohmann::json::object();
    for (const auto& [tool_name, tool_policy] : settings.tools) {
        // Base policy structure
        nlohmann::json create_policy = {
            {"allowed_directories", tool_policy.create.allowed_directories},
            {"confirmation_required", tool_policy.create.confirmation_required}
        };
        
        // Add command lists for bash_tool
        if (tool_name == "bash_tool") {
            if (!tool_policy.create.allowed_commands.empty()) {
                create_policy["allowed_commands"] = tool_policy.create.allowed_commands;
            }
            if (!tool_policy.create.blocked_commands.empty()) {
                create_policy["blocked_commands"] = tool_policy.create.blocked_commands;
            }
        }
        
        json["tools"][tool_name] = {
            {"create", create_policy},
            {"read", {
                {"allowed_directories", tool_policy.read.allowed_directories},
                {"confirmation_required", tool_policy.read.confirmation_required}
            }},
            {"update", {
                {"allowed_directories", tool_policy.update.allowed_directories},
                {"confirmation_required", tool_policy.update.confirmation_required}
            }},
            {"delete", {
                {"allowed_directories", tool_policy.delete_op.allowed_directories},
                {"confirmation_required", tool_policy.delete_op.confirmation_required}
            }}
        };
    }
    
    return json;
}

std::unique_ptr<PolicySettings> PolicyConfig::json_to_settings(const nlohmann::json& json) {
    auto settings = std::make_unique<PolicySettings>();
    
    // Parse global settings
    const auto& global = json["global"];
    settings->global.blocked_extensions = global["blocked_extensions"].get<std::vector<std::string>>();
    settings->global.max_file_size_mb = global["max_file_size_mb"].get<size_t>();
    settings->global.auto_backup = global["auto_backup"].get<bool>();
    
    // Parse tool settings
    settings->tools.clear();
    for (const auto& [tool_name, tool_data] : json["tools"].items()) {
        ToolPolicy tool_policy;
        
        // Parse each operation
        const auto& create_data = tool_data["create"];
        tool_policy.create.allowed_directories = create_data["allowed_directories"].get<std::vector<std::string>>();
        tool_policy.create.confirmation_required = create_data["confirmation_required"].get<bool>();
        
        // Load command lists for bash_tool
        if (tool_name == "bash_tool") {
            if (create_data.contains("allowed_commands") && create_data["allowed_commands"].is_array()) {
                tool_policy.create.allowed_commands = create_data["allowed_commands"].get<std::vector<std::string>>();
            }
            if (create_data.contains("blocked_commands") && create_data["blocked_commands"].is_array()) {
                tool_policy.create.blocked_commands = create_data["blocked_commands"].get<std::vector<std::string>>();
            }
        }
        
        const auto& read_data = tool_data["read"];
        tool_policy.read.allowed_directories = read_data["allowed_directories"].get<std::vector<std::string>>();
        tool_policy.read.confirmation_required = read_data["confirmation_required"].get<bool>();
        
        const auto& update_data = tool_data["update"];
        tool_policy.update.allowed_directories = update_data["allowed_directories"].get<std::vector<std::string>>();
        tool_policy.update.confirmation_required = update_data["confirmation_required"].get<bool>();
        
        const auto& delete_data = tool_data["delete"];
        tool_policy.delete_op.allowed_directories = delete_data["allowed_directories"].get<std::vector<std::string>>();
        tool_policy.delete_op.confirmation_required = delete_data["confirmation_required"].get<bool>();
        
        settings->tools[tool_name] = tool_policy;
    }
    
    return settings;
}

} // namespace mag