#pragma once

#include <string>

namespace mag {

class Utils {
public:
    static std::string get_real_path(const std::string& path);
    static std::string get_current_working_directory();
    static bool file_exists(const std::string& path);
    static size_t get_file_size(const std::string& content);
    static bool create_directories(const std::string& path);
};

} // namespace mag