#pragma once

#include <string>
#include <memory>
#include <vector>

namespace mag {

/**
 * @brief Abstract interface for handling user input with platform-specific implementations
 * 
 * This abstraction allows us to use readline on Linux/macOS for a professional CLI experience,
 * while falling back to simple stdin on Windows for universal compatibility.
 */
class InputHandler {
public:
    virtual ~InputHandler() = default;
    
    /**
     * @brief Get a line of input from the user
     * @param prompt The prompt to display to the user
     * @return The user's input, or empty string on EOF
     */
    virtual std::string get_line(const std::string& prompt) = 0;
    
    /**
     * @brief Add a line to the command history
     * @param line The command to add to history
     */
    virtual void add_history(const std::string& line) = 0;
    
    /**
     * @brief Save command history to persistent storage
     */
    virtual void save_history() = 0;
    
    /**
     * @brief Load command history from persistent storage
     */
    virtual void load_history() = 0;
    
    /**
     * @brief Set up tab completion (if supported by implementation)
     * @param completions List of possible completions
     */
    virtual void setup_completion(const std::vector<std::string>& completions) = 0;
    
    /**
     * @brief Check if this handler supports advanced features like tab completion
     * @return true if advanced features are supported
     */
    virtual bool supports_advanced_features() const = 0;
};

/**
 * @brief Factory function to create the appropriate InputHandler for the current platform
 * @return Unique pointer to the best available InputHandler implementation
 */
std::unique_ptr<InputHandler> create_input_handler();

} // namespace mag