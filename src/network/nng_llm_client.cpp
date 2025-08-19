#include "network/nng_llm_client.h"
#include "config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <regex>

namespace mag {

NNGLLMClient::NNGLLMClient(const std::string& provider_override) 
    : llm_socket_(nullptr), current_provider_(provider_override) {
    initialize_socket();
}

NNGLLMClient::~NNGLLMClient() {
    cleanup_socket();
}

void NNGLLMClient::initialize_socket() {
    int rv;
    
    // Create LLM adapter socket
    if ((rv = nng_req0_open(reinterpret_cast<nng_socket*>(&llm_socket_))) != 0) {
        throw std::runtime_error("Failed to open LLM socket: " + std::string(nng_strerror(rv)));
    }
    
    // Connect to LLM adapter
    std::string llm_url = NetworkConfig::get_llm_adapter_url();
    if ((rv = nng_dial(*reinterpret_cast<nng_socket*>(&llm_socket_), llm_url.c_str(), nullptr, 0)) != 0) {
        nng_close(*reinterpret_cast<nng_socket*>(&llm_socket_));
        throw std::runtime_error("Failed to connect to LLM adapter: " + std::string(nng_strerror(rv)));
    }
}

void NNGLLMClient::cleanup_socket() {
    if (llm_socket_) {
        nng_close(*reinterpret_cast<nng_socket*>(&llm_socket_));
        llm_socket_ = nullptr;
    }
}

WriteFileCommand NNGLLMClient::request_plan(const std::string& user_prompt) {
    int rv;
    
    // Create request with optional provider override
    nlohmann::json request = {
        {"prompt", user_prompt}
    };
    
    if (!current_provider_.empty()) {
        request["provider"] = current_provider_;
    }
    
    std::string request_str = request.dump();
    
    // Send request
    if ((rv = nng_send(*reinterpret_cast<nng_socket*>(&llm_socket_), 
                      const_cast<char*>(request_str.c_str()), request_str.length(), 0)) != 0) {
        throw std::runtime_error("Failed to send to LLM adapter: " + std::string(nng_strerror(rv)));
    }
    
    // Receive response
    char* buf = nullptr;
    size_t sz;
    if ((rv = nng_recv(*reinterpret_cast<nng_socket*>(&llm_socket_), &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
        throw std::runtime_error("Failed to receive from LLM adapter: " + std::string(nng_strerror(rv)));
    }
    
    std::string response(buf, sz);
    nng_free(buf, sz);
    
    return MessageHandler::deserialize_command(response);
}

GenericCommand NNGLLMClient::request_generic_plan(const std::string& user_prompt) {
    // For legacy NNG implementation, fallback to file operations
    WriteFileCommand legacy_cmd = request_plan(user_prompt);
    GenericCommand generic_cmd;
    generic_cmd.type = OperationType::FILE_WRITE;
    generic_cmd.description = legacy_cmd.command + " " + legacy_cmd.path;
    generic_cmd.file_path = legacy_cmd.path;
    generic_cmd.file_content = legacy_cmd.content;
    return generic_cmd;
}

std::string NNGLLMClient::request_chat(const std::string& user_prompt) {
    int rv;
    
    // Create request with chat mode indicator
    nlohmann::json request = {
        {"prompt", user_prompt},
        {"chat_mode", true}
    };
    
    if (!current_provider_.empty()) {
        request["provider"] = current_provider_;
    }
    
    std::string request_str = request.dump();
    
    // Send request
    if ((rv = nng_send(*reinterpret_cast<nng_socket*>(&llm_socket_), 
                      const_cast<char*>(request_str.c_str()), request_str.length(), 0)) != 0) {
        throw std::runtime_error("Failed to send to LLM adapter: " + std::string(nng_strerror(rv)));
    }
    
    // Receive response
    char* buf = nullptr;
    size_t sz;
    if ((rv = nng_recv(*reinterpret_cast<nng_socket*>(&llm_socket_), &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
        throw std::runtime_error("Failed to receive from LLM adapter: " + std::string(nng_strerror(rv)));
    }
    
    std::string response(buf, sz);
    nng_free(buf, sz);
    
    return response;
}

void NNGLLMClient::set_provider(const std::string& provider_name) {
    // Map friendly names to internal names
    if (provider_name == "chatgpt") {
        current_provider_ = "openai";
    } else if (provider_name == "claude") {
        current_provider_ = "anthropic";
    } else {
        current_provider_ = provider_name; // gemini, mistral use same names
    }
}

std::string NNGLLMClient::get_current_provider() const {
    return current_provider_;
}

std::string NNGLLMClient::regex_escape(const std::string& str) {
    static const std::regex special_chars{R"([-[\]{}()*+?.,\^$|#\s])"};
    return std::regex_replace(str, special_chars, R"(\$&)");
}

} // namespace mag