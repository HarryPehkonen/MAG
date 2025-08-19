#include "conversation_manager.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace mag {

ConversationManager::ConversationManager() 
    : storage_directory_(".mag/conversations") {
    start_new_session();
}

ConversationManager::ConversationManager(const std::string& session_id) 
    : session_id_(session_id), storage_directory_(".mag/conversations") {
    if (!load_session(session_id)) {
        start_new_session(session_id);
    }
}

ConversationManager::~ConversationManager() {
    if (!conversation_history_.empty()) {
        try {
            save_to_disk();
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to save conversation on destruction: " << e.what() << std::endl;
        }
    }
}

void ConversationManager::add_user_message(const std::string& content) {
    conversation_history_.emplace_back("user", content);
    update_last_activity();
}

void ConversationManager::add_assistant_message(const std::string& content, const std::string& provider) {
    conversation_history_.emplace_back("assistant", content, provider);
    last_provider_used_ = provider;
    update_last_activity();
}

void ConversationManager::add_system_message(const std::string& content) {
    conversation_history_.emplace_back("system", content);
    update_last_activity();
}

std::vector<ConversationMessage> ConversationManager::get_history() const {
    return conversation_history_;
}

std::vector<ConversationMessage> ConversationManager::get_history_since(const std::string& timestamp) const {
    std::vector<ConversationMessage> result;
    for (const auto& msg : conversation_history_) {
        if (msg.timestamp >= timestamp) {
            result.push_back(msg);
        }
    }
    return result;
}

size_t ConversationManager::get_message_count() const {
    return conversation_history_.size();
}

bool ConversationManager::is_empty() const {
    return conversation_history_.empty();
}

void ConversationManager::clear_history() {
    conversation_history_.clear();
    update_last_activity();
}

void ConversationManager::trim_to_last_n_messages(size_t n) {
    if (conversation_history_.size() > n) {
        conversation_history_.erase(
            conversation_history_.begin(), 
            conversation_history_.end() - n
        );
        update_last_activity();
    }
}

void ConversationManager::trim_to_token_limit(size_t max_tokens) {
    size_t estimated_tokens = 0;
    auto it = conversation_history_.rbegin();
    
    // Count from the end to keep the most recent messages
    while (it != conversation_history_.rend() && 
           estimated_tokens < max_tokens) {
        estimated_tokens += it->content.length() / CHARS_PER_TOKEN;
        ++it;
    }
    
    // Remove messages from the beginning if we exceeded the limit
    if (it != conversation_history_.rend()) {
        size_t messages_to_keep = std::distance(it, conversation_history_.rend());
        conversation_history_.erase(
            conversation_history_.begin(),
            conversation_history_.begin() + (conversation_history_.size() - messages_to_keep)
        );
        update_last_activity();
    }
}

void ConversationManager::start_new_session() {
    start_new_session(generate_session_id());
}

void ConversationManager::start_new_session(const std::string& session_id) {
    // Save current session if it has content
    if (!conversation_history_.empty()) {
        save_to_disk();
    }
    
    session_id_ = session_id;
    conversation_history_.clear();
    session_created_time_ = ConversationMessage::get_current_timestamp();
    last_activity_time_ = session_created_time_;
    last_provider_used_ = "";
}

std::string ConversationManager::get_current_session_id() const {
    return session_id_;
}

void ConversationManager::save_to_disk() {
    if (conversation_history_.empty()) {
        return; // Don't save empty conversations
    }
    
    try {
        ensure_storage_directory_exists();
        
        std::string file_path = get_session_file_path();
        std::ofstream file(file_path);
        if (!file) {
            throw std::runtime_error("Failed to open conversation file for writing: " + file_path);
        }
        
        nlohmann::json j = to_json();
        file << j.dump(2) << std::endl;
        
        std::cout << "Conversation saved to: " << file_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving conversation: " << e.what() << std::endl;
        throw;
    }
}

void ConversationManager::load_from_disk() {
    load_session(session_id_);
}

bool ConversationManager::load_session(const std::string& session_id) {
    try {
        std::string file_path = get_session_file_path(session_id);
        
        if (!std::filesystem::exists(file_path)) {
            return false;
        }
        
        std::ifstream file(file_path);
        if (!file) {
            std::cerr << "Warning: Failed to open conversation file: " << file_path << std::endl;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        from_json(j);
        session_id_ = session_id;
        
        std::cout << "Loaded conversation session: " << session_id << 
                     " (" << conversation_history_.size() << " messages)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading conversation session " << session_id << ": " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> ConversationManager::get_available_sessions() const {
    std::vector<std::string> sessions;
    
    try {
        if (!std::filesystem::exists(storage_directory_)) {
            return sessions;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(storage_directory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().stem().string();
                sessions.push_back(filename);
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(sessions.begin(), sessions.end(), [this](const std::string& a, const std::string& b) {
            auto time_a = std::filesystem::last_write_time(get_session_file_path(a));
            auto time_b = std::filesystem::last_write_time(get_session_file_path(b));
            return time_a > time_b;
        });
    } catch (const std::exception& e) {
        std::cerr << "Error getting available sessions: " << e.what() << std::endl;
    }
    
    return sessions;
}

void ConversationManager::set_storage_directory(const std::string& dir) {
    storage_directory_ = dir;
}

std::string ConversationManager::get_storage_directory() const {
    return storage_directory_;
}

std::string ConversationManager::get_session_created_time() const {
    return session_created_time_;
}

std::string ConversationManager::get_last_activity_time() const {
    return last_activity_time_;
}

std::string ConversationManager::get_last_provider_used() const {
    return last_provider_used_;
}

// Private methods

std::string ConversationManager::generate_session_id() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "session_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string ConversationManager::get_session_file_path() const {
    return get_session_file_path(session_id_);
}

std::string ConversationManager::get_session_file_path(const std::string& session_id) const {
    return storage_directory_ + "/" + session_id + ".json";
}

void ConversationManager::update_last_activity() {
    last_activity_time_ = ConversationMessage::get_current_timestamp();
}

void ConversationManager::ensure_storage_directory_exists() {
    try {
        std::filesystem::create_directories(storage_directory_);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create storage directory " + storage_directory_ + ": " + e.what());
    }
}

nlohmann::json ConversationManager::to_json() const {
    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : conversation_history_) {
        messages.push_back(msg.to_json());
    }
    
    return nlohmann::json{
        {"session_id", session_id_},
        {"created", session_created_time_},
        {"last_activity", last_activity_time_},
        {"last_provider", last_provider_used_},
        {"message_count", conversation_history_.size()},
        {"messages", messages}
    };
}

void ConversationManager::from_json(const nlohmann::json& j) {
    conversation_history_.clear();
    
    if (j.contains("messages") && j["messages"].is_array()) {
        for (const auto& msg_json : j["messages"]) {
            conversation_history_.push_back(ConversationMessage::from_json(msg_json));
        }
    }
    
    if (j.contains("created")) {
        session_created_time_ = j["created"];
    }
    if (j.contains("last_activity")) {
        last_activity_time_ = j["last_activity"];
    }
    if (j.contains("last_provider")) {
        last_provider_used_ = j["last_provider"];
    }
}

} // namespace mag