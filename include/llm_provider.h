#pragma once

#include "message.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace mag {

// Conversation message for chat history
struct ConversationMessage {
    std::string role;      // "user", "assistant", "system"
    std::string content;   // Message content
    std::string timestamp; // ISO 8601 timestamp
    std::string provider;  // Provider that generated this message (for assistant messages)
    
    ConversationMessage(const std::string& r, const std::string& c) 
        : role(r), content(c), timestamp(get_current_timestamp()), provider("") {}
    
    ConversationMessage(const std::string& r, const std::string& c, const std::string& p) 
        : role(r), content(c), timestamp(get_current_timestamp()), provider(p) {}
    
    // JSON serialization support
    nlohmann::json to_json() const;
    static ConversationMessage from_json(const nlohmann::json& j);
    
    // Utility methods
    static std::string get_current_timestamp();
};

// Abstract base class for LLM providers
class LLMProvider {
public:
    virtual ~LLMProvider() = default;
    
    // Provider identification
    virtual std::string get_name() const = 0;
    virtual std::string get_api_url() const = 0;
    virtual std::string get_default_model() const = 0;
    
    // URL construction with API key (default implementation uses base URL)
    virtual std::string get_full_url(const std::string& api_key, const std::string& model = "") const {
        return get_api_url();
    }
    
    // Request building
    virtual nlohmann::json build_request_payload(
        const std::string& system_prompt,
        const std::string& user_prompt,
        const std::string& model
    ) const = 0;
    
    // Request building with conversation history
    virtual nlohmann::json build_conversation_payload(
        const std::string& system_prompt,
        const std::vector<ConversationMessage>& conversation_history,
        const std::string& model
    ) const {
        // Default implementation: use only the last user message for backward compatibility
        if (!conversation_history.empty() && conversation_history.back().role == "user") {
            return build_request_payload(system_prompt, conversation_history.back().content, model);
        }
        return build_request_payload(system_prompt, "", model);
    }
    
    // Header configuration
    virtual std::vector<std::string> get_headers(const std::string& api_key) const = 0;
    
    // Response parsing
    virtual WriteFileCommand parse_response(const std::string& response) const = 0;
    virtual std::string parse_chat_response(const std::string& response) const {
        // Default implementation: return placeholder
        return "Chat response parsing not implemented for this provider";
    }
    
    // Environment variable for API key
    virtual std::string get_api_key_env_var() const = 0;
};

// Factory for creating providers
class ProviderFactory {
public:
    static std::unique_ptr<LLMProvider> create_provider(const std::string& provider_name);
    static std::string detect_available_provider();
    static std::vector<std::string> get_supported_providers();
    
private:
    static void register_providers();
    static std::vector<std::string> supported_providers_;
};

} // namespace mag