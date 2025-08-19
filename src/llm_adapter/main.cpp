#include "llm_client.h"
#include "message.h"
#include "config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace mag;

int main() {
    nng_socket sock;
    int rv;
    
    try {
        // Initialize LLM client with auto-detection
        LLMClient llm_client;
        
        std::cout << "Using " << llm_client.get_current_provider() 
                  << " with model " << llm_client.get_current_model() << std::endl;
        
        // Create NNG reply socket
        if ((rv = nng_rep0_open(&sock)) != 0) {
            std::cerr << "nng_rep0_open: " << nng_strerror(rv) << std::endl;
            return 1;
        }
        
        // Listen on the specified URL
        std::string url = NetworkConfig::get_llm_adapter_url();
        if ((rv = nng_listen(sock, url.c_str(), nullptr, 0)) != 0) {
            std::cerr << "nng_listen: " << nng_strerror(rv) << std::endl;
            nng_close(sock);
            return 1;
        }
        
        std::cout << "LLM Adapter listening on " << url << std::endl;
        
        // Main service loop
        while (true) {
        char* buf = nullptr;
        size_t sz;
        
        // Receive message
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
            std::cerr << "nng_recv: " << nng_strerror(rv) << std::endl;
            continue;
        }
        
        try {
            // Parse the request (could be plain string or JSON)
            std::string request_data(buf, sz);
            nng_free(buf, sz);
            
            std::string user_prompt;
            std::string provider_override;
            bool chat_mode = false;
            
            // Try to parse as JSON first
            try {
                nlohmann::json request_json = nlohmann::json::parse(request_data);
                user_prompt = request_json["prompt"];
                if (request_json.contains("provider")) {
                    provider_override = request_json["provider"];
                }
                if (request_json.contains("chat_mode")) {
                    chat_mode = request_json["chat_mode"];
                }
                std::cout << "Received JSON request - Prompt: " << user_prompt;
                if (!provider_override.empty()) {
                    std::cout << ", Provider: " << provider_override;
                }
                if (chat_mode) {
                    std::cout << ", Mode: chat";
                }
                std::cout << std::endl;
            } catch (const nlohmann::json::exception&) {
                // Not JSON, treat as plain prompt
                user_prompt = request_data;
                std::cout << "Received prompt: " << user_prompt << std::endl;
            }
            
            if (chat_mode) {
                // Chat mode - return raw response
                std::string chat_response;
                if (!provider_override.empty()) {
                    // Temporarily switch provider on existing client
                    std::string original_provider = llm_client.get_current_provider();
                    llm_client.set_provider(provider_override);
                    chat_response = llm_client.get_chat_response(user_prompt);
                    llm_client.set_provider(original_provider); // Switch back
                    std::cout << "Used provider override: " << provider_override << std::endl;
                } else {
                    chat_response = llm_client.get_chat_response(user_prompt);
                }
                
                std::cout << "Chat response: " << chat_response << std::endl;
                
                // Send raw response
                if ((rv = nng_send(sock, const_cast<char*>(chat_response.c_str()), chat_response.length(), 0)) != 0) {
                    std::cerr << "nng_send: " << nng_strerror(rv) << std::endl;
                } else {
                    std::cout << "Sent chat response" << std::endl;
                }
            } else {
                // File operation mode - parse as WriteFileCommand
                WriteFileCommand command;
                if (!provider_override.empty()) {
                    // Temporarily switch provider on existing client
                    std::string original_provider = llm_client.get_current_provider();
                    llm_client.set_provider(provider_override);
                    command = llm_client.get_plan_from_llm(user_prompt);
                    llm_client.set_provider(original_provider); // Switch back
                    std::cout << "Used provider override: " << provider_override << std::endl;
                } else {
                    command = llm_client.get_plan_from_llm(user_prompt);
                }
                
                std::cout << "LLM response parsed - Command: '" << command.command 
                          << "', Path: '" << command.path 
                          << "', Content length: " << command.content.length() << std::endl;
                
                // Serialize and send response
                std::string response = MessageHandler::serialize_command(command);
                
                if ((rv = nng_send(sock, const_cast<char*>(response.c_str()), response.length(), 0)) != 0) {
                    std::cerr << "nng_send: " << nng_strerror(rv) << std::endl;
                } else {
                    std::cout << "Sent plan: " << response << std::endl;
                }
            }
            
        } catch (const std::exception& e) {
            nng_free(buf, sz);
            std::cerr << "Error processing request: " << e.what() << std::endl;
            
            // Send error response
            std::string error_response = R"({"command": "WriteFile", "path": "", "content": ""})";
            nng_send(sock, const_cast<char*>(error_response.c_str()), error_response.length(), 0);
        }
        }
        
        nng_close(sock);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}