#include "bash_tool.h"
#include "message.h"
#include "config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <iostream>
#include <cstring>
#include <nlohmann/json.hpp>

using namespace mag;

/**
 * @brief Bash Tool Service - Handles bash command execution with persistent state
 * 
 * This service maintains a persistent working directory between command executions,
 * enabling complex workflows like "cd build && make" to work across multiple todos.
 */
class BashToolService {
public:
    BashToolService() : bash_tool_() {
        // Initialize with current working directory
        current_working_directory_ = bash_tool_.get_current_directory();
        std::cout << "Bash Tool Service initialized with working directory: " 
                  << current_working_directory_ << std::endl;
    }
    
    std::string handle_request(const std::string& request_data) {
        try {
            nlohmann::json request_json = nlohmann::json::parse(request_data);
            
            std::string operation = request_json["operation"];
            
            if (operation == "execute") {
                return handle_execute_command(request_json);
            } else if (operation == "get_pwd") {
                return handle_get_pwd();
            } else if (operation == "set_pwd") {
                return handle_set_pwd(request_json);
            } else {
                throw std::runtime_error("Unknown operation: " + operation);
            }
            
        } catch (const std::exception& e) {
            return create_error_response("Request handling error: " + std::string(e.what()));
        }
    }
    
private:
    BashTool bash_tool_;
    std::string current_working_directory_;
    
    std::string handle_execute_command(const nlohmann::json& request) {
        try {
            std::string command = request["command"];
            std::string working_dir = current_working_directory_;
            
            // If a specific working directory is provided, use it
            if (request.contains("working_directory") && !request["working_directory"].empty()) {
                working_dir = request["working_directory"];
            }
            
            std::cout << "Executing command: " << command 
                      << " in directory: " << working_dir << std::endl;
            
            // Execute command with context capture
            CommandResult result = bash_tool_.execute_command(command, working_dir);
            
            // Update persistent working directory from result
            if (!result.pwd_after_execution.empty()) {
                current_working_directory_ = result.pwd_after_execution;
                std::cout << "Updated working directory to: " << current_working_directory_ << std::endl;
            }
            
            // Convert CommandResult to JSON response
            nlohmann::json response;
            response["success"] = result.success;
            response["exit_code"] = result.exit_code;
            response["stdout_output"] = result.stdout_output;
            response["stderr_output"] = result.stderr_output;
            response["working_directory_before"] = result.working_directory;
            response["working_directory_after"] = result.pwd_after_execution;
            response["execution_duration_ms"] = result.execution_duration.count();
            
            return response.dump();
            
        } catch (const std::exception& e) {
            return create_error_response("Command execution error: " + std::string(e.what()));
        }
    }
    
    std::string handle_get_pwd() {
        nlohmann::json response;
        response["success"] = true;
        response["working_directory"] = current_working_directory_;
        return response.dump();
    }
    
    std::string handle_set_pwd(const nlohmann::json& request) {
        try {
            std::string new_directory = request["working_directory"];
            
            // Validate directory exists (optional safety check)
            // For now, just accept it
            current_working_directory_ = new_directory;
            
            nlohmann::json response;
            response["success"] = true;
            response["working_directory"] = current_working_directory_;
            return response.dump();
            
        } catch (const std::exception& e) {
            return create_error_response("Set working directory error: " + std::string(e.what()));
        }
    }
    
    std::string create_error_response(const std::string& error_message) {
        nlohmann::json error_response;
        error_response["success"] = false;
        error_response["error_message"] = error_message;
        return error_response.dump();
    }
};

void handle_request(const std::string& request_data, nng_socket sock, BashToolService& service) {
    try {
        std::string response = service.handle_request(request_data);
        
        // Send response
        int rv = nng_send(sock, const_cast<char*>(response.c_str()), response.length(), 0);
        if (rv != 0) {
            std::cerr << "nng_send: " << nng_strerror(rv) << std::endl;
        } else {
            std::cout << "Sent response" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        
        // Send error response
        std::string error_response = "{\"success\": false, \"error_message\": \"" + 
                                   std::string(e.what()) + "\"}";
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
    
    // Listen on the bash tool URL (port 5557)
    std::string url = "tcp://127.0.0.1:5557";
    if ((rv = nng_listen(sock, url.c_str(), nullptr, 0)) != 0) {
        std::cerr << "nng_listen: " << nng_strerror(rv) << std::endl;
        nng_close(sock);
        return 1;
    }
    
    std::cout << "Bash Tool Service listening on " << url << std::endl;
    
    // Create service instance
    BashToolService service;
    
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
        
        handle_request(request_data, sock, service);
    }
    
    nng_close(sock);
    return 0;
}