#pragma once

#include "message.h"
#include <string>

namespace mag {

class FileTool {
public:
    FileTool() = default;
    
    DryRunResult dry_run(const std::string& path, const std::string& content) const;
    ApplyResult apply(const std::string& path, const std::string& content) const;
    
private:
    std::string generate_dry_run_description(const std::string& path, const std::string& content) const;
    std::string generate_apply_description(const std::string& path, const std::string& content) const;
};

} // namespace mag