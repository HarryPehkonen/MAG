#pragma once

#include "llm_provider.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace mag {

class ConversationManager {
public:
    ConversationManager();
    explicit ConversationManager(const std::string& session_id);
    ~ConversationManager();

    // Core conversation operations
    void add_user_message(const std::string& content);
    void add_assistant_message(const std::string& content, const std::string& provider);
    void add_system_message(const std::string& content);
    
    // History access
    std::vector<ConversationMessage> get_history() const;
    std::vector<ConversationMessage> get_history_since(const std::string& timestamp) const;
    size_t get_message_count() const;
    bool is_empty() const;
    
    // History management
    void clear_history();
    void trim_to_last_n_messages(size_t n);
    void trim_to_token_limit(size_t max_tokens); // Rough estimation
    
    // Session management
    void start_new_session();
    void start_new_session(const std::string& session_id);
    std::string get_current_session_id() const;
    
    // Persistence
    void save_to_disk();
    void load_from_disk();
    bool load_session(const std::string& session_id);
    std::vector<std::string> get_available_sessions() const;
    
    // Configuration
    void set_storage_directory(const std::string& dir);
    std::string get_storage_directory() const;
    
    // Metadata
    std::string get_session_created_time() const;
    std::string get_last_activity_time() const;
    std::string get_last_provider_used() const;

private:
    std::vector<ConversationMessage> conversation_history_;
    std::string session_id_;
    std::string storage_directory_;
    std::string session_created_time_;
    std::string last_activity_time_;
    std::string last_provider_used_;
    
    // Helper methods
    std::string generate_session_id() const;
    std::string get_session_file_path() const;
    std::string get_session_file_path(const std::string& session_id) const;
    void update_last_activity();
    void ensure_storage_directory_exists();
    
    // JSON serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
    // Constants
    static constexpr size_t DEFAULT_TOKEN_LIMIT = 8000; // Conservative estimate
    static constexpr size_t CHARS_PER_TOKEN = 4; // Rough approximation
};

} // namespace mag