#pragma once

#include "coordinator.h"
#include "input_handler.h"
#include "conversation_manager.h"
#include <memory>
#include <fstream>
#include <string>

namespace mag {

/**
 * @brief Clean, professional command-line interface for MAG
 * 
 * Provides a readline-based CLI on Linux/macOS with full features:
 * - Command history and tab completion
 * - Provider switching commands
 * - Chat mode with todo integration
 * - Debug logging and status commands
 * 
 * Falls back to simple stdin on Windows for universal compatibility.
 */
class CLIInterface {
public:
    CLIInterface();
    CLIInterface(const std::string& provider_override);
    ~CLIInterface();
    
    /**
     * @brief Run the interactive CLI loop
     */
    void run();
    
private:
    Coordinator coordinator_;
    std::unique_ptr<InputHandler> input_handler_;
    std::unique_ptr<ConversationManager> conversation_manager_;
    std::ofstream debug_log_;
    bool running_;
    
    /**
     * @brief Handle a single command from the user
     * @param input The user's input
     */
    void handle_command(const std::string& input);
    
    /**
     * @brief Handle slash commands (provider switching, help, etc.)
     * @param command The command without the leading slash
     */
    void handle_slash_command(const std::string& command);
    
    /**
     * @brief Show the welcome message and help
     */
    void show_welcome();
    
    /**
     * @brief Show help information
     */
    void show_help();
    
    /**
     * @brief Show system status
     */
    void show_status();
    
    /**
     * @brief Show debug information
     */
    void show_debug();
    
    /**
     * @brief Initialize debug logging
     */
    void init_debug_log();
    
    /**
     * @brief Set up tab completion for commands
     */
    void setup_completion();
    
    /**
     * @brief Get the current prompt string
     * @return Formatted prompt
     */
    std::string get_prompt() const;
    
    /**
     * @brief Print colored output (if terminal supports it)
     * @param text The text to print
     * @param color Color code (e.g., "32" for green, "31" for red)
     */
    void print_colored(const std::string& text, const std::string& color = "");
    
    /**
     * @brief Check if the terminal supports colors
     * @return true if colors are supported
     */
    bool supports_colors() const;
    
    /**
     * @brief Show the current todo list
     */
    void show_todo_list();
    
    /**
     * @brief Show execution status
     */
    void show_execution_status();
    
    /**
     * @brief Handle do commands for todo management
     * @param command The do command (after "do")
     */
    void handle_do_command(const std::string& command);
    
    /**
     * @brief Handle provider switching with conversation context
     * @param provider_name The new provider to switch to
     */
    void switch_provider_with_context(const std::string& provider_name);
    
    /**
     * @brief Show conversation history
     */
    void show_conversation_history();
    
    /**
     * @brief Handle conversation session commands
     * @param command The session command
     */
    void handle_session_command(const std::string& command);
};

} // namespace mag