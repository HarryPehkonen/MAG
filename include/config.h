#pragma once

#include <string>

namespace mag {

// Network configuration
struct NetworkConfig {
    static constexpr const char* LLM_ADAPTER_HOST = "127.0.0.1";
    static constexpr int LLM_ADAPTER_PORT = 5555;
    static constexpr const char* FILE_TOOL_HOST = "127.0.0.1";
    static constexpr int FILE_TOOL_PORT = 5556;
    static constexpr const char* BASH_TOOL_HOST = "127.0.0.1";
    static constexpr int BASH_TOOL_PORT = 5557;
    
    static std::string get_llm_adapter_url() {
        return std::string("tcp://") + LLM_ADAPTER_HOST + ":" + std::to_string(LLM_ADAPTER_PORT);
    }
    
    static std::string get_file_tool_url() {
        return std::string("tcp://") + FILE_TOOL_HOST + ":" + std::to_string(FILE_TOOL_PORT);
    }
    
    static std::string get_bash_tool_url() {
        return std::string("tcp://") + BASH_TOOL_HOST + ":" + std::to_string(BASH_TOOL_PORT);
    }
};

// API configuration
struct APIConfig {
    // Gemini API
    static constexpr const char* GEMINI_BASE_URL = "https://generativelanguage.googleapis.com/v1beta/models";
    static constexpr const char* GEMINI_DEFAULT_MODEL = "gemini-1.5-flash";
    
    // OpenAI API
    static constexpr const char* OPENAI_BASE_URL = "https://api.openai.com/v1/chat/completions";
    static constexpr const char* OPENAI_DEFAULT_MODEL = "gpt-3.5-turbo";
    
    // Anthropic API
    static constexpr const char* ANTHROPIC_BASE_URL = "https://api.anthropic.com/v1/messages";
    static constexpr const char* ANTHROPIC_DEFAULT_MODEL = "claude-3-haiku-20240307";
    
    // Mistral API
    static constexpr const char* MISTRAL_BASE_URL = "https://api.mistral.ai/v1/chat/completions";
    static constexpr const char* MISTRAL_DEFAULT_MODEL = "mistral-tiny";
};

} // namespace mag