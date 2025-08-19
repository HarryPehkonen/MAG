#pragma once

#include "llm_provider.h"

namespace mag {

class GeminiProvider : public LLMProvider {
public:
    std::string get_name() const override;
    std::string get_api_url() const override;
    std::string get_default_model() const override;
    std::string get_full_url(const std::string& api_key, const std::string& model = "") const override;
    
    nlohmann::json build_request_payload(
        const std::string& system_prompt,
        const std::string& user_prompt,
        const std::string& model
    ) const override;
    
    nlohmann::json build_conversation_payload(
        const std::string& system_prompt,
        const std::vector<ConversationMessage>& conversation_history,
        const std::string& model
    ) const override;
    
    std::vector<std::string> get_headers(const std::string& api_key) const override;
    WriteFileCommand parse_response(const std::string& response) const override;
    std::string parse_chat_response(const std::string& response) const override;
    std::string get_api_key_env_var() const override;
};

} // namespace mag