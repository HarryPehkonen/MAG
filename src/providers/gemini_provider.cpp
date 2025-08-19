#include "providers/gemini_provider.h"
#include "config.h"
#include <stdexcept>
#include <iostream>

namespace mag {

std::string GeminiProvider::get_name() const {
    return "gemini";
}

std::string GeminiProvider::get_api_url() const {
    return std::string(APIConfig::GEMINI_BASE_URL) + "/" + APIConfig::GEMINI_DEFAULT_MODEL + ":generateContent";
}

std::string GeminiProvider::get_full_url(const std::string& api_key, const std::string& model) const {
    std::string actual_model = model.empty() ? get_default_model() : model;
    std::string base_url = std::string(APIConfig::GEMINI_BASE_URL) + "/" + actual_model + ":generateContent";
    return base_url + "?key=" + api_key;
}

std::string GeminiProvider::get_default_model() const {
    return APIConfig::GEMINI_DEFAULT_MODEL;
}

nlohmann::json GeminiProvider::build_request_payload(
    const std::string& system_prompt,
    const std::string& user_prompt,
    const std::string& model
) const {
    // Gemini API format - combines system and user prompts
    std::string combined_prompt = system_prompt + "\n\nUser: " + user_prompt;
    
    return nlohmann::json{
        {"contents", {
            {{"parts", {
                {{"text", combined_prompt}}
            }}}
        }},
        {"generationConfig", {
            {"temperature", 0.1},
            {"maxOutputTokens", 1000}
        }}
    };
}

nlohmann::json GeminiProvider::build_conversation_payload(
    const std::string& system_prompt,
    const std::vector<ConversationMessage>& conversation_history,
    const std::string& model
) const {
    nlohmann::json contents = nlohmann::json::array();
    
    // Convert conversation history to Gemini format
    for (const auto& msg : conversation_history) {
        nlohmann::json message = {
            {"parts", {
                {{"text", msg.content}}
            }},
            {"role", msg.role == "assistant" ? "model" : msg.role}  // Gemini uses "model" instead of "assistant"
        };
        contents.push_back(message);
    }
    
    return nlohmann::json{
        {"contents", contents},
        {"systemInstruction", {
            {"parts", {
                {{"text", system_prompt}}
            }},
            {"role", "user"}
        }},
        {"generationConfig", {
            {"temperature", 0.1},
            {"maxOutputTokens", 1000}
        }}
    };
}

std::vector<std::string> GeminiProvider::get_headers(const std::string& api_key) const {
    return {
        "Content-Type: application/json"
    };
}

WriteFileCommand GeminiProvider::parse_response(const std::string& response) const {
    std::cout << "Gemini Provider: Parsing response..." << std::endl;
    std::cout << "Response length: " << response.length() << std::endl;
    std::cout << "First 200 chars: " << response.substr(0, 200) << std::endl;
    
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Gemini API response format
        if (json_response.contains("candidates") && json_response["candidates"].is_array() && 
            !json_response["candidates"].empty()) {
            
            auto candidate = json_response["candidates"][0];
            if (candidate.contains("content") && candidate["content"].contains("parts") &&
                candidate["content"]["parts"].is_array() && !candidate["content"]["parts"].empty()) {
                
                std::string content = candidate["content"]["parts"][0]["text"];
                std::cout << "Extracted text content: " << content << std::endl;
                
                // Remove markdown code block formatting if present
                std::string json_content = content;
                
                // Look for ```json and ``` markers
                size_t start_pos = json_content.find("```json");
                if (start_pos != std::string::npos) {
                    start_pos += 7; // Length of "```json"
                    // Skip any whitespace/newlines after ```json
                    while (start_pos < json_content.length() && 
                           (json_content[start_pos] == '\n' || json_content[start_pos] == '\r' || json_content[start_pos] == ' ')) {
                        start_pos++;
                    }
                    
                    size_t end_pos = json_content.find("```", start_pos);
                    if (end_pos != std::string::npos) {
                        json_content = json_content.substr(start_pos, end_pos - start_pos);
                    }
                }
                
                std::cout << "Cleaned JSON content: " << json_content << std::endl;
                
                // Parse the content as JSON to get the WriteFile command
                nlohmann::json command_json = nlohmann::json::parse(json_content);
                
                WriteFileCommand command;
                command.from_json(command_json);
                
                return command;
            }
        }
        
        throw std::runtime_error("Invalid Gemini API response format");
        
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Gemini response: " + std::string(e.what()));
    }
}

std::string GeminiProvider::parse_chat_response(const std::string& response) const {
    try {
        nlohmann::json json_response = nlohmann::json::parse(response);
        
        // Extract the content from Gemini API response format
        if (json_response.contains("candidates") && json_response["candidates"].is_array() && 
            !json_response["candidates"].empty()) {
            
            auto candidate = json_response["candidates"][0];
            if (candidate.contains("content") && candidate["content"].contains("parts") &&
                candidate["content"]["parts"].is_array() && !candidate["content"]["parts"].empty()) {
                
                std::string content = candidate["content"]["parts"][0]["text"];
                return content; // Return raw text for chat mode
            }
        }
        
        throw std::runtime_error("Invalid Gemini API response format");
        
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse Gemini chat response: " + std::string(e.what()));
    }
}

std::string GeminiProvider::get_api_key_env_var() const {
    return "GEMINI_API_KEY";
}

} // namespace mag