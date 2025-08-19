#include "readline_input_handler.h"
#include "utils.h"

#ifdef HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <iostream>
#include <cstdlib>

namespace mag {

#ifdef HAS_READLINE

// Static member initialization
ReadlineInputHandler* ReadlineInputHandler::current_instance_ = nullptr;

ReadlineInputHandler::ReadlineInputHandler() {
    current_instance_ = this;
    
    // Set up history file path
    std::string mag_dir = Utils::get_current_working_directory() + "/.mag";
    history_file_ = mag_dir + "/history";
    
    // Create .mag directory if it doesn't exist
    Utils::create_directories(mag_dir);
    
    // Set up readline
    rl_attempted_completion_function = completion_callback;
    
    // Load existing history
    load_history();
}

ReadlineInputHandler::~ReadlineInputHandler() {
    save_history();
    current_instance_ = nullptr;
}

std::string ReadlineInputHandler::get_line(const std::string& prompt) {
    char* line = readline(prompt.c_str());
    
    if (line == nullptr) {
        // EOF (Ctrl+D)
        return "";
    }
    
    std::string result(line);
    free(line);
    
    return result;
}

void ReadlineInputHandler::add_history(const std::string& line) {
    if (!line.empty() && line != "exit" && line != "quit") {
        ::add_history(line.c_str());
    }
}

void ReadlineInputHandler::save_history() {
    write_history(history_file_.c_str());
}

void ReadlineInputHandler::load_history() {
    read_history(history_file_.c_str());
    
    // Limit history size
    stifle_history(1000);
}

void ReadlineInputHandler::setup_completion(const std::vector<std::string>& completions) {
    completion_list_ = completions;
}

char** ReadlineInputHandler::completion_callback(const char* text, int start, int end) {
    // Don't use default filename completion
    rl_attempted_completion_over = 1;
    
    return rl_completion_matches(text, completion_generator);
}

char* ReadlineInputHandler::completion_generator(const char* text, int state) {
    static size_t list_index, len;
    
    if (!current_instance_) {
        return nullptr;
    }
    
    // Initialize on first call (state == 0)
    if (state == 0) {
        list_index = 0;
        len = strlen(text);
    }
    
    // Return next match
    while (list_index < current_instance_->completion_list_.size()) {
        const std::string& completion = current_instance_->completion_list_[list_index++];
        
        if (completion.length() >= len && 
            completion.compare(0, len, text) == 0) {
            // readline will free this memory
            char* match = static_cast<char*>(malloc(completion.length() + 1));
            strcpy(match, completion.c_str());
            return match;
        }
    }
    
    return nullptr;
}

#else

// Fallback implementation when readline is not available
ReadlineInputHandler::ReadlineInputHandler() {
    std::cerr << "Warning: ReadlineInputHandler created but readline not available. "
              << "This should not happen - check build configuration." << std::endl;
}

ReadlineInputHandler::~ReadlineInputHandler() = default;

std::string ReadlineInputHandler::get_line(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void ReadlineInputHandler::add_history(const std::string& line) {
    // No-op in fallback
}

void ReadlineInputHandler::save_history() {
    // No-op in fallback
}

void ReadlineInputHandler::load_history() {
    // No-op in fallback
}

void ReadlineInputHandler::setup_completion(const std::vector<std::string>& completions) {
    // No-op in fallback
}

#endif

} // namespace mag