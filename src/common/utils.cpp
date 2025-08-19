#include "utils.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace mag {

std::string Utils::get_real_path(const std::string& path) {
    try {
        std::filesystem::path fs_path(path);
        // First try canonical (works for existing paths)
        return std::filesystem::canonical(fs_path).string();
    } catch (const std::filesystem::filesystem_error&) {
        // If canonical fails (e.g., file doesn't exist), resolve manually
        std::filesystem::path abs_path = std::filesystem::absolute(path);
        // Use lexically_normal to resolve .. and . components
        return abs_path.lexically_normal().string();
    }
}

std::string Utils::get_current_working_directory() {
    return std::filesystem::current_path().string();
}

bool Utils::file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

size_t Utils::get_file_size(const std::string& content) {
    return content.size();
}

bool Utils::create_directories(const std::string& file_path) {
    try {
        std::filesystem::path fs_path(file_path);
        std::filesystem::path parent_path = fs_path.parent_path();
        
        if (parent_path.empty()) {
            return true; // No parent directory needed
        }
        
        // create_directories returns false if directory already exists
        // So we need to check if it exists or was successfully created
        if (std::filesystem::exists(parent_path)) {
            return true;
        }
        
        return std::filesystem::create_directories(parent_path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

} // namespace mag