#include "llm_provider.h"
#include "providers/openai_provider.h"
#include "providers/anthropic_provider.h"
#include "providers/gemini_provider.h"
#include "providers/mistral_provider.h"
#include <cstdlib>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace mag {

std::vector<std::string> ProviderFactory::supported_providers_;

std::unique_ptr<LLMProvider> ProviderFactory::create_provider(const std::string& provider_name) {
    if (provider_name == "openai") {
        return std::make_unique<OpenAIProvider>();
    } else if (provider_name == "anthropic") {
        return std::make_unique<AnthropicProvider>();
    } else if (provider_name == "gemini") {
        return std::make_unique<GeminiProvider>();
    } else if (provider_name == "mistral") {
        return std::make_unique<MistralProvider>();
    } else {
        throw std::runtime_error("Unsupported LLM provider: " + provider_name);
    }
}

std::string ProviderFactory::detect_available_provider() {
    // Check for available API keys in order of preference
    const char* anthropic_key = std::getenv("ANTHROPIC_API_KEY");
    if (anthropic_key && strlen(anthropic_key) > 0) {
        return "anthropic";
    }
    
    const char* openai_key = std::getenv("OPENAI_API_KEY");
    if (openai_key && strlen(openai_key) > 0) {
        return "openai";
    }
    
    const char* gemini_key = std::getenv("GEMINI_API_KEY");
    if (gemini_key && strlen(gemini_key) > 0) {
        return "gemini";
    }
    
    const char* mistral_key = std::getenv("MISTRAL_API_KEY");
    if (mistral_key && strlen(mistral_key) > 0) {
        return "mistral";
    }
    
    throw std::runtime_error("No supported LLM provider API key found. Please set one of: ANTHROPIC_API_KEY, OPENAI_API_KEY, GEMINI_API_KEY, MISTRAL_API_KEY");
}

std::vector<std::string> ProviderFactory::get_supported_providers() {
    if (supported_providers_.empty()) {
        register_providers();
    }
    return supported_providers_;
}

void ProviderFactory::register_providers() {
    supported_providers_ = {
        "anthropic",
        "openai",
        "gemini",     // Future implementation
        "mistral"     // Future implementation
    };
}

// ConversationMessage implementations
std::string ConversationMessage::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

nlohmann::json ConversationMessage::to_json() const {
    return nlohmann::json{
        {"role", role},
        {"content", content},
        {"timestamp", timestamp},
        {"provider", provider}
    };
}

ConversationMessage ConversationMessage::from_json(const nlohmann::json& j) {
    ConversationMessage msg(j.at("role"), j.at("content"));
    if (j.contains("timestamp")) {
        msg.timestamp = j.at("timestamp");
    }
    if (j.contains("provider")) {
        msg.provider = j.at("provider");
    }
    return msg;
}

} // namespace mag