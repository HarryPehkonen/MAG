#pragma once

#include "message.h"
#include "llm_provider.h"
#include "http_client.h"
#include <string>
#include <memory>

namespace mag {

class LLMClient {
public:
    // Constructor with auto-detection
    LLMClient();
    
    // Constructor with specific provider
    LLMClient(const std::string& provider_name, const std::string& api_key = "", 
              const std::string& model = "");
    
    WriteFileCommand get_plan_from_llm(const std::string& user_prompt) const;
    std::string get_chat_response(const std::string& user_prompt) const;
    std::string get_chat_response_with_history(const std::vector<ConversationMessage>& conversation_history) const;
    
    // Information methods
    std::string get_current_provider() const;
    std::string get_current_model() const;
    
    // Provider management
    void set_provider(const std::string& provider_name, const std::string& model = "");
    
private:
    std::unique_ptr<LLMProvider> provider_;
    std::string api_key_;
    std::string model_;
    std::string system_prompt_;
    HttpClient http_client_;
    
    void initialize_provider(const std::string& provider_name, const std::string& api_key, 
                            const std::string& model);
    std::string get_api_key_for_provider(const std::string& provider_name) const;
    std::string generate_policy_aware_system_prompt() const;
};

} // namespace mag