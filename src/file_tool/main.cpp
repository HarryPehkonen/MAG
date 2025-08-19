#include "file_operations.h"
#include "message.h"
#include "config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <iostream>
#include <cstring>

using namespace mag;

struct RequestMessage {
    std::string operation; // "dry_run" or "apply"
    WriteFileCommand command;
};

void handle_request(const std::string& request_data, nng_socket sock) {
    FileTool file_tool;
    
    try {
        // Parse the request
        nlohmann::json request_json = nlohmann::json::parse(request_data);
        
        std::string operation = request_json["operation"];
        WriteFileCommand command;
        command.from_json(request_json["command"]);
        
        std::string response;
        
        if (operation == "dry_run") {
            DryRunResult result = file_tool.dry_run(command.path, command.content);
            response = MessageHandler::serialize_dry_run_result(result);
        } else if (operation == "apply") {
            ApplyResult result = file_tool.apply(command.path, command.content);
            response = MessageHandler::serialize_apply_result(result);
        } else {
            throw std::runtime_error("Unknown operation: " + operation);
        }
        
        // Send response
        int rv = nng_send(sock, const_cast<char*>(response.c_str()), response.length(), 0);
        if (rv != 0) {
            std::cerr << "nng_send: " << nng_strerror(rv) << std::endl;
        } else {
            std::cout << "Sent " << operation << " result" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        
        // Send error response
        std::string error_response;
        if (request_data.find("dry_run") != std::string::npos) {
            DryRunResult error_result;
            error_result.success = false;
            error_result.error_message = e.what();
            error_result.description = "";
            error_response = MessageHandler::serialize_dry_run_result(error_result);
        } else {
            ApplyResult error_result;
            error_result.success = false;
            error_result.error_message = e.what();
            error_result.description = "";
            error_response = MessageHandler::serialize_apply_result(error_result);
        }
        
        nng_send(sock, const_cast<char*>(error_response.c_str()), error_response.length(), 0);
    }
}

int main() {
    nng_socket sock;
    int rv;
    
    // Create NNG reply socket
    if ((rv = nng_rep0_open(&sock)) != 0) {
        std::cerr << "nng_rep0_open: " << nng_strerror(rv) << std::endl;
        return 1;
    }
    
    // Listen on the specified URL
    std::string url = NetworkConfig::get_file_tool_url();
    if ((rv = nng_listen(sock, url.c_str(), nullptr, 0)) != 0) {
        std::cerr << "nng_listen: " << nng_strerror(rv) << std::endl;
        nng_close(sock);
        return 1;
    }
    
    std::cout << "File Tool listening on " << url << std::endl;
    
    // Main service loop
    while (true) {
        char* buf = nullptr;
        size_t sz;
        
        // Receive message
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
            std::cerr << "nng_recv: " << nng_strerror(rv) << std::endl;
            continue;
        }
        
        std::string request_data(buf, sz);
        nng_free(buf, sz);
        
        std::cout << "Received request: " << request_data << std::endl;
        
        handle_request(request_data, sock);
    }
    
    nng_close(sock);
    return 0;
}