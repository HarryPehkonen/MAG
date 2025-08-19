#pragma once

#include "input_handler.h"
#include <vector>
#include <string>

namespace mag {

/**
 * @brief Professional readline-based input handler for Linux/macOS
 * 
 * Provides full readline features including:
 * - Command history with persistent storage
 * - Tab completion for commands
 * - Line editing (arrow keys, Ctrl+A/E, etc.)
 * - History search (Ctrl+R)
 * - Multi-line input support
 */
class ReadlineInputHandler : public InputHandler {
public:
    ReadlineInputHandler();
    ~ReadlineInputHandler() override;
    
    std::string get_line(const std::string& prompt) override;
    void add_history(const std::string& line) override;
    void save_history() override;
    void load_history() override;
    void setup_completion(const std::vector<std::string>& completions) override;
    bool supports_advanced_features() const override { return true; }

private:
    std::string history_file_;
    std::vector<std::string> completion_list_;
    
    // Static callback for readline tab completion
    static char** completion_callback(const char* text, int start, int end);
    static char* completion_generator(const char* text, int state);
    
    // Static pointer to current instance for callback access
    static ReadlineInputHandler* current_instance_;
};

} // namespace mag