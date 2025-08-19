#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <nlohmann/json.hpp>

namespace mag {

// CRUD operation types
enum class Operation {
    CREATE,
    READ,
    UPDATE,
    DELETE
};

// Policy for a specific tool operation
struct OperationPolicy {
    std::vector<std::string> allowed_directories;
    bool confirmation_required;
    std::vector<std::string> allowed_commands;   // For bash_tool
    std::vector<std::string> blocked_commands;   // For bash_tool
    
    OperationPolicy() : confirmation_required(true) {}
    OperationPolicy(const std::vector<std::string>& dirs, bool confirm = true)
        : allowed_directories(dirs), confirmation_required(confirm) {}
};

// Tool-specific policies
struct ToolPolicy {
    OperationPolicy create;
    OperationPolicy read;
    OperationPolicy update;
    OperationPolicy delete_op; // 'delete' is a C++ keyword
    
    ToolPolicy() = default;
};

// Global policy settings
struct GlobalPolicy {
    std::vector<std::string> blocked_extensions;
    size_t max_file_size_mb;
    bool auto_backup;
    
    GlobalPolicy() 
        : blocked_extensions({".key", ".pem", ".env", ".secret", ".crt"})
        , max_file_size_mb(10)
        , auto_backup(false) {}
};

struct PolicySettings {
    GlobalPolicy global;
    std::map<std::string, ToolPolicy> tools;
    
    // Default constructor with hardcoded defaults
    PolicySettings();
    
    // Validation
    bool validate(std::string& error_message) const;
    
    // Helper methods
    const OperationPolicy* get_operation_policy(const std::string& tool, Operation op) const;
    bool is_operation_allowed(const std::string& tool, Operation op, const std::string& path) const;
};

class PolicyConfig {
public:
    // Main entry point - loads existing config or creates default
    static std::unique_ptr<PolicySettings> load_or_create();
    
    // Save policy settings to .mag/policy.json
    static bool save(const PolicySettings& settings, std::string& error_message);
    
    // Get .mag directory path
    static std::string get_mag_directory();
    
    // Get policy.json file path
    static std::string get_policy_file_path();
    
private:
    // Create .mag/ directory if it doesn't exist
    static bool ensure_mag_directory_exists(std::string& error_message);
    
    // Create default policy.json file
    static bool create_default_config(std::string& error_message);
    
    // Parse existing policy.json file
    static std::unique_ptr<PolicySettings> parse_config(const std::string& file_path, std::string& error_message);
    
    // Validate JSON schema
    static bool validate_json_schema(const nlohmann::json& json, std::string& error_message);
    
    // Convert PolicySettings to JSON
    static nlohmann::json settings_to_json(const PolicySettings& settings);
    
    // Convert JSON to PolicySettings
    static std::unique_ptr<PolicySettings> json_to_settings(const nlohmann::json& json);
};

} // namespace mag