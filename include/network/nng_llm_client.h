#pragma once

#include "interfaces/llm_client_interface.h"
#include <string>

namespace mag {

/**
 * @brief NNG-based implementation of LLM client interface
 * 
 * This class implements the ILLMClient interface using NNG sockets
 * to communicate with the LLM adapter service.
 */
class NNGLLMClient : public ILLMClient {
public:
    /**
     * @brief Constructor
     * @param provider_override Optional provider to use instead of auto-detection
     */
    explicit NNGLLMClient(const std::string& provider_override = "");
    
    /**
     * @brief Destructor - cleans up NNG socket
     */
    ~NNGLLMClient() override;
    
    // ILLMClient interface implementation
    WriteFileCommand request_plan(const std::string& user_prompt) override;
    GenericCommand request_generic_plan(const std::string& user_prompt) override;
    std::string request_chat(const std::string& user_prompt) override;
    void set_provider(const std::string& provider_name) override;
    std::string get_current_provider() const override;
    
private:
    void* llm_socket_;
    std::string current_provider_;
    
    void initialize_socket();
    void cleanup_socket();
    
    // Helper function to escape regex special characters
    std::string regex_escape(const std::string& str);
};

} // namespace mag