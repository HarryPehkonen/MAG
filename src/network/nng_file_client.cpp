#include "network/nng_file_client.h"
#include "config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace mag {

NNGFileClient::NNGFileClient() : file_socket_(nullptr) {
    initialize_socket();
}

NNGFileClient::~NNGFileClient() {
    cleanup_socket();
}

void NNGFileClient::initialize_socket() {
    int rv;
    
    // Create file tool socket
    if ((rv = nng_req0_open(reinterpret_cast<nng_socket*>(&file_socket_))) != 0) {
        throw std::runtime_error("Failed to open file tool socket: " + std::string(nng_strerror(rv)));
    }
    
    // Connect to file tool
    std::string file_url = NetworkConfig::get_file_tool_url();
    if ((rv = nng_dial(*reinterpret_cast<nng_socket*>(&file_socket_), file_url.c_str(), nullptr, 0)) != 0) {
        nng_close(*reinterpret_cast<nng_socket*>(&file_socket_));
        throw std::runtime_error("Failed to connect to file tool: " + std::string(nng_strerror(rv)));
    }
}

void NNGFileClient::cleanup_socket() {
    if (file_socket_) {
        nng_close(*reinterpret_cast<nng_socket*>(&file_socket_));
        file_socket_ = nullptr;
    }
}

DryRunResult NNGFileClient::dry_run(const WriteFileCommand& command) {
    nlohmann::json request = {
        {"operation", "dry_run"},
        {"command", {
            {"command", command.command},
            {"path", command.path},
            {"content", command.content}
        }}
    };
    
    std::string request_str = request.dump();
    int rv;
    
    // Send request
    if ((rv = nng_send(*reinterpret_cast<nng_socket*>(&file_socket_), 
                      const_cast<char*>(request_str.c_str()), request_str.length(), 0)) != 0) {
        throw std::runtime_error("Failed to send to file tool: " + std::string(nng_strerror(rv)));
    }
    
    // Receive response
    char* buf = nullptr;
    size_t sz;
    if ((rv = nng_recv(*reinterpret_cast<nng_socket*>(&file_socket_), &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
        throw std::runtime_error("Failed to receive from file tool: " + std::string(nng_strerror(rv)));
    }
    
    std::string response(buf, sz);
    nng_free(buf, sz);
    
    return MessageHandler::deserialize_dry_run_result(response);
}

ApplyResult NNGFileClient::apply(const WriteFileCommand& command) {
    nlohmann::json request = {
        {"operation", "apply"},
        {"command", {
            {"command", command.command},
            {"path", command.path},
            {"content", command.content}
        }}
    };
    
    std::string request_str = request.dump();
    int rv;
    
    // Send request
    if ((rv = nng_send(*reinterpret_cast<nng_socket*>(&file_socket_), 
                      const_cast<char*>(request_str.c_str()), request_str.length(), 0)) != 0) {
        throw std::runtime_error("Failed to send to file tool: " + std::string(nng_strerror(rv)));
    }
    
    // Receive response
    char* buf = nullptr;
    size_t sz;
    if ((rv = nng_recv(*reinterpret_cast<nng_socket*>(&file_socket_), &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
        throw std::runtime_error("Failed to receive from file tool: " + std::string(nng_strerror(rv)));
    }
    
    std::string response(buf, sz);
    nng_free(buf, sz);
    
    return MessageHandler::deserialize_apply_result(response);
}

} // namespace mag