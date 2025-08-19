#include "file_operations.h"
#include "utils.h"
#include "bash_tool.h"
#include <fstream>
#include <iostream>
#include <chrono>

namespace mag {

DryRunResult FileTool::dry_run(const std::string& path, const std::string& content) const {
    DryRunResult result;
    
    try {
        result.description = generate_dry_run_description(path, content);
        result.success = true;
        result.error_message = "";
    } catch (const std::exception& e) {
        result.description = "";
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

ApplyResult FileTool::apply(const std::string& path, const std::string& content) const {
    ApplyResult result;
    
    // Capture execution context before operation
    BashTool bash_tool;
    std::string working_dir_before = bash_tool.get_current_directory();
    
    try {
        // Create parent directories if they don't exist
        if (!Utils::create_directories(path)) {
            throw std::runtime_error("Failed to create parent directories");
        }
        
        // Write the file
        std::ofstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + path);
        }
        
        file << content;
        file.close();
        
        if (file.fail()) {
            throw std::runtime_error("Failed to write content to file: " + path);
        }
        
        result.description = generate_apply_description(path, content);
        result.success = true;
        result.error_message = "";
        
    } catch (const std::exception& e) {
        result.description = "";
        result.success = false;
        result.error_message = e.what();
    }
    
    // Capture execution context after operation
    result.execution_context.working_directory_before = working_dir_before;
    result.execution_context.working_directory_after = bash_tool.get_current_directory();
    result.execution_context.exit_code = result.success ? 0 : 1;
    result.execution_context.timestamp = std::chrono::system_clock::now();
    
    // For file operations, add the file path and size to the output
    if (result.success) {
        result.execution_context.command_output = "Created file: " + path + " (" + 
                                                 std::to_string(content.size()) + " bytes)";
    }
    
    return result;
}

std::string FileTool::generate_dry_run_description(const std::string& path, const std::string& content) const {
    size_t content_size = Utils::get_file_size(content);
    
    if (Utils::file_exists(path)) {
        return "[DRY-RUN] Will overwrite existing file '" + path + "' with " + 
               std::to_string(content_size) + " bytes.";
    } else {
        return "[DRY-RUN] Will create new file '" + path + "' with " + 
               std::to_string(content_size) + " bytes.";
    }
}

std::string FileTool::generate_apply_description(const std::string& path, const std::string& content) const {
    size_t content_size = Utils::get_file_size(content);
    return "[APPLIED] Successfully wrote " + std::to_string(content_size) + " bytes to '" + path + "'.";
}

} // namespace mag