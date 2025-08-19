#pragma once

#include "policy_config.h"
#include <string>
#include <vector>
#include <memory>

namespace mag {

class PolicyChecker {
public:
    PolicyChecker();
    
    // Check if operation is allowed for tool+operation+path combination
    bool is_allowed(const std::string& tool, Operation operation, const std::string& path) const;
    
    // Legacy method for backward compatibility
    bool is_allowed(const std::string& path) const;
    bool is_extension_blocked(const std::string& path) const;
    bool is_file_size_allowed(size_t size_bytes) const;
    
    // Get current policy settings
    const PolicySettings& get_settings() const { return *settings_; }
    
    // Get allowed directories for a specific tool and operation
    std::vector<std::string> get_allowed_directories(const std::string& tool, const std::string& operation) const;
    
    // Bash command validation
    bool is_bash_command_allowed(const std::string& command) const;
    bool is_bash_command_blocked(const std::string& command) const;
    std::string get_bash_command_violation_reason(const std::string& command) const;
    
    // Update policy settings and save to file
    bool update_settings(const PolicySettings& new_settings, std::string& error_message);
    
private:
    std::unique_ptr<PolicySettings> settings_;
    
    bool is_within_cwd(const std::string& path) const;
    bool has_allowed_prefix(const std::string& tool, Operation operation, const std::string& path) const;
    bool has_allowed_prefix(const std::string& path) const; // Legacy method
};

} // namespace mag