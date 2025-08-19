#include "simple_input_handler.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace mag {

SimpleInputHandler::SimpleInputHandler() {
    // Set up history file path
    std::string mag_dir = Utils::get_current_working_directory() + "/.mag";
    history_file_ = mag_dir + "/history";
    
    // Create .mag directory if it doesn't exist
    Utils::create_directories(mag_dir);
    
    // Load existing history
    load_history();
}

SimpleInputHandler::~SimpleInputHandler() {
    save_history();
}

std::string SimpleInputHandler::get_line(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    
    std::string line;
    if (std::getline(std::cin, line)) {
        return line;
    } else {
        // EOF (Ctrl+D on Unix, Ctrl+Z on Windows)
        return "";
    }
}

void SimpleInputHandler::add_history(const std::string& line) {
    if (line.empty() || line == "exit" || line == "quit") {
        return;
    }
    
    // Remove duplicates - if the same command is already at the end, don't add again
    if (!history_.empty() && history_.back() == line) {
        return;
    }
    
    history_.push_back(line);
    
    // Limit history size
    if (history_.size() > MAX_HISTORY_SIZE) {
        history_.pop_front();
    }
}

void SimpleInputHandler::save_history() {
    std::ofstream file(history_file_);
    if (!file.is_open()) {
        return; // Silently fail if we can't save history
    }
    
    for (const auto& line : history_) {
        file << line << '\n';
    }
}

void SimpleInputHandler::load_history() {
    std::ifstream file(history_file_);
    if (!file.is_open()) {
        return; // No existing history file
    }
    
    std::string line;
    while (std::getline(file, line) && history_.size() < MAX_HISTORY_SIZE) {
        if (!line.empty()) {
            history_.push_back(line);
        }
    }
}

void SimpleInputHandler::setup_completion(const std::vector<std::string>& completions) {
    completion_list_ = completions;
}

std::string SimpleInputHandler::handle_tab_completion(const std::string& input) {
    // For simple handler, we just show available completions
    // This would typically be called when user presses Tab, but since we're using
    // std::getline, we can't easily detect Tab. This is mainly for future enhancement.
    
    auto matches = find_completions(input);
    
    if (matches.empty()) {
        return input;
    }
    
    if (matches.size() == 1) {
        // Single match - could auto-complete (but we don't have Tab detection in std::getline)
        return input;
    }
    
    // Multiple matches - show them
    std::cout << "\nAvailable completions:\n";
    for (const auto& match : matches) {
        std::cout << "  " << match << "\n";
    }
    
    return input;
}

std::vector<std::string> SimpleInputHandler::find_completions(const std::string& prefix) {
    std::vector<std::string> matches;
    
    for (const auto& completion : completion_list_) {
        if (completion.length() >= prefix.length() && 
            completion.compare(0, prefix.length(), prefix) == 0) {
            matches.push_back(completion);
        }
    }
    
    return matches;
}

} // namespace mag