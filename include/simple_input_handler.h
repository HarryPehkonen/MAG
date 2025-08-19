#pragma once

#include "input_handler.h"
#include <vector>
#include <string>
#include <deque>

namespace mag {

/**
 * @brief Simple stdin-based input handler for Windows and fallback
 * 
 * Provides basic CLI functionality using standard C++ streams:
 * - Basic input with prompts using std::getline()
 * - Manual command history stored in memory
 * - Simple tab completion (shows available options)
 * - Cross-platform compatibility without external dependencies
 */
class SimpleInputHandler : public InputHandler {
public:
    SimpleInputHandler();
    ~SimpleInputHandler() override;
    
    std::string get_line(const std::string& prompt) override;
    void add_history(const std::string& line) override;
    void save_history() override;
    void load_history() override;
    void setup_completion(const std::vector<std::string>& completions) override;
    bool supports_advanced_features() const override { return false; }

private:
    std::string history_file_;
    std::deque<std::string> history_;
    std::vector<std::string> completion_list_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;
    
    /**
     * @brief Handle simple tab completion by showing available options
     * @param input Current input text
     * @return Modified input (usually unchanged for simple handler)
     */
    std::string handle_tab_completion(const std::string& input);
    
    /**
     * @brief Find completions that match the given prefix
     * @param prefix The text to complete
     * @return Vector of matching completions
     */
    std::vector<std::string> find_completions(const std::string& prefix);
};

} // namespace mag