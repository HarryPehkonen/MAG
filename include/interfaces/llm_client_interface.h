#pragma once

#include "message.h"
#include <string>

namespace mag {

/**
 * @brief Interface for LLM communication
 * 
 * This interface abstracts the communication with LLM providers,
 * making the Coordinator testable and provider-agnostic.
 */
class ILLMClient {
public:
    virtual ~ILLMClient() = default;
    
    /**
     * @brief Request a file operation plan from the LLM
     * @param user_prompt The user's request
     * @return WriteFileCommand containing the planned operation
     * @throws std::runtime_error on communication failure
     */
    virtual WriteFileCommand request_plan(const std::string& user_prompt) = 0;
    
    /**
     * @brief Request a generic operation (file or bash) from the LLM
     * @param user_prompt The user's request
     * @return GenericCommand containing the planned operation
     * @throws std::runtime_error on communication failure
     */
    virtual GenericCommand request_generic_plan(const std::string& user_prompt) = 0;
    
    /**
     * @brief Request a chat response from the LLM
     * @param user_prompt The user's message
     * @return LLM's chat response (may contain todo operations)
     * @throws std::runtime_error on communication failure
     */
    virtual std::string request_chat(const std::string& user_prompt) = 0;
    
    /**
     * @brief Set the LLM provider to use
     * @param provider_name Provider name (anthropic, openai, gemini, mistral)
     */
    virtual void set_provider(const std::string& provider_name) = 0;
    
    /**
     * @brief Get the current provider name
     * @return Current provider name
     */
    virtual std::string get_current_provider() const = 0;
};

} // namespace mag