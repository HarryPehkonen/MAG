#include "providers/anthropic_provider.h"
#include <stdexcept>

namespace mag {

std::string AnthropicProvider::get_name() const {
    return "anthropic";
}

std::string AnthropicProvider::get_api_url() const {
    return "https://api.anthropic.com/v1/messages";
}

std::string AnthropicProvider::get_default_model() const {
    return "claude-3-haiku-20240307";
}

nlohmann::json AnthropicProvider::build_request_payload(
    const std::string& system_prompt,
    const std::string& user_prompt,
    const std::string& model
) const {
    return nlohmann::json{
        {"model", model},
        {"max_tokens", 1000},
        {"temperature", 0.1},
        {"system", system_prompt},
        {"messages", {
            {{"role", "user"}, {"content", {{{"type", "text"}, {"text", user_prompt}}}}}
        }}
    };
}

nlohmann::json AnthropicProvider::build_conversation_payload(
    const std::string& system_prompt,
    const std::vector<ConversationMessage>& conversation_history,
    const std::string& model
) const {
    nlohmann::json messages = nlohmann::json::array();
    
    // Convert conversation history to Anthropic format
    for (const auto& msg : conversation_history) {
        nlohmann::json message = {
            {"role", msg.role},
            {"content", {{{"type", "text"}, {"text", msg.content}}}}
        };
        messages.push_back(message);
    }
    
    return nlohmann::json{
        {"model", model},
        {"max_tokens", 1000},
        {"temperature", 0.1},
        {"system", system_prompt},
        {"messages", messages}
    };
}

std::vector<std::string> AnthropicProvider::get_headers(const std::string& api_key) const {
    return {
        "Content-Type: application/json",
        "anthropic-version: 2023-06-01",
        "x-api-key: " + api_key
    };
}

WriteFileCommand AnthropicProvider::parse_response(const std::string& response) const {
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Anthropic API response format
        if (json_response.contains("content") && json_response["content"].is_array() && 
            !json_response["content"].empty()) {
            
            std::string content = json_response["content"][0]["text"];
            
            // Parse the content as JSON to get the WriteFile command
            nlohmann::json command_json = nlohmann::json::parse(content);
            
            WriteFileCommand command;
            command.from_json(command_json);
            
            return command;
        } else {
            throw std::runtime_error("Invalid Anthropic API response format");
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Anthropic response: " + std::string(e.what()));
    }
}

std::string AnthropicProvider::parse_chat_response(const std::string& response) const {
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Anthropic API response format
        if (json_response.contains("content") && json_response["content"].is_array() && 
            !json_response["content"].empty()) {
            
            std::string content = json_response["content"][0]["text"];
            return content; // Return raw text for chat mode
        } else {
            throw std::runtime_error("Invalid Anthropic API response format");
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Anthropic chat response: " + std::string(e.what()));
    }
}

std::string AnthropicProvider::get_api_key_env_var() const {
    return "ANTHROPIC_API_KEY";
}

} // namespace mag