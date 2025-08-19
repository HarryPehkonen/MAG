#include "providers/mistral_provider.h"
#include <stdexcept>

namespace mag {

std::string MistralProvider::get_name() const {
    return "mistral";
}

std::string MistralProvider::get_api_url() const {
    return "https://api.mistral.ai/v1/chat/completions";
}

std::string MistralProvider::get_default_model() const {
    return "mistral-small-latest";
}

nlohmann::json MistralProvider::build_request_payload(
    const std::string& system_prompt,
    const std::string& user_prompt,
    const std::string& model
) const {
    // Mistral uses OpenAI-compatible format
    return nlohmann::json{
        {"model", model},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt}},
            {{"role", "user"}, {"content", user_prompt}}
        }},
        {"max_tokens", 1000},
        {"temperature", 0.1}
    };
}

nlohmann::json MistralProvider::build_conversation_payload(
    const std::string& system_prompt,
    const std::vector<ConversationMessage>& conversation_history,
    const std::string& model
) const {
    nlohmann::json messages = nlohmann::json::array();
    
    // Add system message first
    messages.push_back({
        {"role", "system"},
        {"content", system_prompt}
    });
    
    // Convert conversation history to Mistral format
    for (const auto& msg : conversation_history) {
        nlohmann::json message = {
            {"role", msg.role},
            {"content", msg.content}
        };
        messages.push_back(message);
    }
    
    return nlohmann::json{
        {"model", model},
        {"messages", messages},
        {"max_tokens", 1000},
        {"temperature", 0.1}
    };
}

std::vector<std::string> MistralProvider::get_headers(const std::string& api_key) const {
    return {
        "Content-Type: application/json",
        "Authorization: Bearer " + api_key
    };
}

WriteFileCommand MistralProvider::parse_response(const std::string& response) const {
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Mistral API response format (OpenAI-compatible)
        if (json_response.contains("choices") && json_response["choices"].is_array() && 
            !json_response["choices"].empty()) {
            
            std::string content = json_response["choices"][0]["message"]["content"];
            
            // Parse the content as JSON to get the WriteFile command
            nlohmann::json command_json = nlohmann::json::parse(content);
            
            WriteFileCommand command;
            command.from_json(command_json);
            
            return command;
        } else {
            throw std::runtime_error("Invalid Mistral API response format");
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Mistral response: " + std::string(e.what()));
    }
}

std::string MistralProvider::parse_chat_response(const std::string& response) const {
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Mistral API response format (OpenAI-compatible)
        if (json_response.contains("choices") && json_response["choices"].is_array() && 
            !json_response["choices"].empty()) {
            
            std::string content = json_response["choices"][0]["message"]["content"];
            return content; // Return raw text for chat mode
        } else {
            throw std::runtime_error("Invalid Mistral API response format");
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Mistral chat response: " + std::string(e.what()));
    }
}

std::string MistralProvider::get_api_key_env_var() const {
    return "MISTRAL_API_KEY";
}

} // namespace mag