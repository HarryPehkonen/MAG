#include "llm_client.h"
#include "policy.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace mag {

LLMClient::LLMClient() {
    // Auto-detect provider
    std::string provider_name = ProviderFactory::detect_available_provider();
    std::string api_key = get_api_key_for_provider(provider_name);
    initialize_provider(provider_name, api_key, "");
}

LLMClient::LLMClient(const std::string& provider_name, const std::string& api_key, 
                     const std::string& model) {
    std::string actual_api_key = api_key.empty() ? get_api_key_for_provider(provider_name) : api_key;
    initialize_provider(provider_name, actual_api_key, model);
}

void LLMClient::initialize_provider(const std::string& provider_name, const std::string& api_key, 
                                   const std::string& model) {
    provider_ = ProviderFactory::create_provider(provider_name);
    api_key_ = api_key;
    model_ = model.empty() ? provider_->get_default_model() : model;
    
    // Generate policy-aware system prompt
    system_prompt_ = generate_policy_aware_system_prompt();
}

std::string LLMClient::generate_policy_aware_system_prompt() const {
    std::string base_prompt = "You are a helpful AI assistant that converts user requests into a single, specific JSON command. You must only respond with a JSON object. Do not add any conversational text or markdown formatting around the JSON.\n\n"
                             "You can use TWO types of commands:\n"
                             "1. \"WriteFile\" - for creating/editing files\n"
                             "2. \"BashCommand\" - for executing shell commands\n\n"
                             "Choose WriteFile for: file creation, editing, content manipulation\n"
                             "Choose BashCommand for: building, testing, running commands, system operations\n\n";
    
    // Try to load current policy constraints
    try {
        PolicyChecker policy;
        auto allowed_dirs = policy.get_allowed_directories("file_tool", "create");
        
        if (!allowed_dirs.empty()) {
            base_prompt += "IMPORTANT POLICY CONSTRAINTS:\n\n";
            base_prompt += "FILE OPERATIONS:\n";
            base_prompt += "- You can ONLY create files in these directories: ";
            for (size_t i = 0; i < allowed_dirs.size(); ++i) {
                base_prompt += allowed_dirs[i];
                if (i < allowed_dirs.size() - 1) base_prompt += ", ";
            }
            base_prompt += "\n";
            base_prompt += "- Files in other directories are NOT allowed\n";
            base_prompt += "- If the user requests a file outside allowed directories, suggest an alternative in one of the allowed directories\n\n";
            
            base_prompt += "BASH COMMANDS:\n";
            
            // Load bash command policies
            auto bash_allowed = policy.get_allowed_directories("bash_tool", "create");
            auto bash_tool_it = policy.get_settings().tools.find("bash_tool");
            if (bash_tool_it != policy.get_settings().tools.end()) {
                const auto& bash_policy = bash_tool_it->second.create;
                
                if (!bash_policy.allowed_commands.empty()) {
                    base_prompt += "- Allowed commands: ";
                    for (size_t i = 0; i < bash_policy.allowed_commands.size(); ++i) {
                        base_prompt += bash_policy.allowed_commands[i];
                        if (i < bash_policy.allowed_commands.size() - 1) base_prompt += ", ";
                    }
                    base_prompt += "\n";
                }
                
                if (!bash_policy.blocked_commands.empty()) {
                    base_prompt += "- Blocked commands: ";
                    for (size_t i = 0; i < bash_policy.blocked_commands.size(); ++i) {
                        base_prompt += bash_policy.blocked_commands[i];
                        if (i < bash_policy.blocked_commands.size() - 1) base_prompt += ", ";
                    }
                    base_prompt += "\n";
                }
            }
            
            base_prompt += "- Commands will be executed in a sandboxed environment with working directory persistence\n\n";
        }
    } catch (const std::exception& e) {
        // If policy loading fails, fall back to basic constraints
        base_prompt += "IMPORTANT POLICY CONSTRAINTS:\n"
                      "FILE OPERATIONS:\n"
                      "- Please create files in appropriate directories (src/, tests/, docs/)\n"
                      "- Avoid creating files in the project root\n\n"
                      "BASH COMMANDS:\n"
                      "- Allowed: make, cmake, gcc, g++, npm, cargo, python, ls, pwd, find, grep, cat, git\n"
                      "- Blocked: rm, dd, mkfs, sudo, chmod 777, shutdown, reboot, curl, wget\n"
                      "- Use safe, standard development commands\n"
                      "- Commands execute in sandboxed environment\n\n";
    }
    
    base_prompt += "JSON FORMAT:\n\n"
                   "For WriteFile commands:\n"
                   "{\n"
                   "  \"command\": \"WriteFile\",\n"
                   "  \"path\": \"relative/path/to/file\",\n"
                   "  \"content\": \"file content here\"\n"
                   "}\n\n"
                   "For BashCommand commands:\n"
                   "{\n"
                   "  \"command\": \"BashCommand\",\n"
                   "  \"bash_command\": \"the shell command to execute\",\n"
                   "  \"description\": \"brief description of what this does\"\n"
                   "}\n\n"
                   "Examples:\n"
                   "User: \"create a python file in src/ called app.py that prints hello world\"\n"
                   "Response: {\"command\": \"WriteFile\", \"path\": \"src/app.py\", \"content\": \"print('Hello, World!')\"}\n\n"
                   "User: \"run make clean to clean the build\"\n"
                   "Response: {\"command\": \"BashCommand\", \"bash_command\": \"make clean\", \"description\": \"Clean build artifacts\"}\n\n"
                   "User: \"execute the python script\"\n"
                   "Response: {\"command\": \"BashCommand\", \"bash_command\": \"python3 src/script.py\", \"description\": \"Execute Python script\"}\n\n"
                   "IMPORTANT: For BashCommand, 'bash_command' must be the EXACT command to execute, not a description!";
    
    return base_prompt;
}

std::string LLMClient::get_api_key_for_provider(const std::string& provider_name) const {
    std::unique_ptr<LLMProvider> temp_provider = ProviderFactory::create_provider(provider_name);
    std::string env_var = temp_provider->get_api_key_env_var();
    
    const char* api_key = std::getenv(env_var.c_str());
    if (!api_key || strlen(api_key) == 0) {
        throw std::runtime_error("API key not found for provider " + provider_name + 
                                ". Please set " + env_var + " environment variable.");
    }
    
    return std::string(api_key);
}

WriteFileCommand LLMClient::get_plan_from_llm(const std::string& user_prompt) const {
    // Build request payload
    nlohmann::json payload = provider_->build_request_payload(system_prompt_, user_prompt, model_);
    
    // Get headers
    std::vector<std::string> headers = provider_->get_headers(api_key_);
    
    std::string url = provider_->get_full_url(api_key_, model_);
    std::string payload_str = payload.dump();
    
    std::cout << "Making request to: " << url << std::endl;
    std::cout << "Request payload: " << payload_str << std::endl;
    
    // Make HTTP request
    HttpResponse response = http_client_.post(url, payload_str, headers);
    
    std::cout << "HTTP Response - Success: " << response.success 
              << ", Status: " << response.status_code << std::endl;
    std::cout << "[LLM-DEBUG] Raw API response: " << response.data << std::endl;
    
    if (!response.success) {
        throw std::runtime_error("HTTP request failed: " + response.error_message + 
                                " (Status: " + std::to_string(response.status_code) + ")");
    }
    
    // Parse response
    WriteFileCommand parsed_command = provider_->parse_response(response.data);
    std::cout << "[LLM-DEBUG] Parsed WriteFileCommand: {" 
              << "\"command\": \"" << parsed_command.command << "\", "
              << "\"path\": \"" << parsed_command.path << "\", "
              << "\"content_length\": " << parsed_command.content.length() << " chars}" << std::endl;
    
    return parsed_command;
}

std::string LLMClient::get_current_provider() const {
    return provider_->get_name();
}

std::string LLMClient::get_current_model() const {
    return model_;
}

void LLMClient::set_provider(const std::string& provider_name, const std::string& model) {
    std::string api_key = get_api_key_for_provider(provider_name);
    initialize_provider(provider_name, api_key, model);
}

std::string LLMClient::get_chat_response(const std::string& user_prompt) const {
    // Todo-centric chat system prompt with tool access
    std::string chat_system_prompt = 
        "You are MAG (Multi-Agent Gateway), a helpful AI assistant with todo management capabilities. "
        "You are currently in CHAT MODE where you can have natural conversations AND manage a todo list.\n\n";
        
    // Add policy constraints dynamically
    try {
        PolicyChecker policy;
        auto allowed_dirs = policy.get_allowed_directories("file_tool", "create");
        
        if (!allowed_dirs.empty()) {
            chat_system_prompt += "IMPORTANT POLICY CONSTRAINTS:\n";
            chat_system_prompt += "- When suggesting file operations, remember that files can ONLY be created in: ";
            for (size_t i = 0; i < allowed_dirs.size(); ++i) {
                chat_system_prompt += allowed_dirs[i];
                if (i < allowed_dirs.size() - 1) chat_system_prompt += ", ";
            }
            chat_system_prompt += "\n";
            chat_system_prompt += "- Files in other directories are NOT allowed\n";
            chat_system_prompt += "- If someone asks for files elsewhere, suggest alternatives in allowed directories\n\n";
        }
    } catch (const std::exception& e) {
        // If policy loading fails, fall back to basic constraints
        chat_system_prompt += "IMPORTANT POLICY CONSTRAINTS:\n"
                             "- Please suggest creating files in appropriate directories\n"
                             "- Avoid suggesting files in the project root\n\n";
    }
        
    chat_system_prompt += "AVAILABLE TOOLS:\n"
        "- add_todo(title, description): Add a new todo item (simple format)\n"
        "- TODO_START/TODO_END blocks: Add complex todos with quotes/special chars\n"
        "- list_todos(): Show current todos\n"
        "- update_todo(id, title, description): Modify existing todo\n"
        "- delete_todo(id): Remove todo item\n"
        "\nWhen creating todos, you can suggest BOTH file operations AND bash commands:\n"
        "- File operations: 'Create config.json with settings', 'Update README.md'\n"
        "- Bash commands: Use EXACT command syntax like 'python3 src/script.py', 'make clean', 'npm install'\n"
        "- For bash todos, be SPECIFIC with executable commands, not descriptions\n"
        "- Examples: 'python3 src/app.py' (not 'run the Python app'), 'ls -la' (not 'list files')\n"
        "- When creating script execution todos, use the exact command: 'python3 src/filename.py'\n"
        "- The system will automatically route each todo to the appropriate tool (file_tool or bash_tool)\n"
        "- mark_complete(id): Mark todo as done\n"
        "\nAUTONOMOUS EXECUTION TOOLS:\n"
        "- execute_next(): Execute the next pending todo autonomously\n"
        "- execute_all(): Execute all pending todos autonomously  \n"
        "- execute_todo(id): Execute a specific todo by ID\n"
        "- request_user_approval(reason): Stop and ask user for approval when uncertain\n\n"
        "EXECUTION COMMANDS (for the user):\n"
        "- /do: Execute all pending todos\n"
        "- /do next: Execute only the next pending todo\n"
        "- /do until N: Execute todos until (not including) ID N\n"
        "- /do N-M: Execute todos in range [start_id, end_id]\n"
        "- /do N: Execute specific todo by ID\n\n"
        
        "AUTONOMOUS EXECUTION GUIDELINES:\n"
        "- Use execute_next() or execute_all() when user clearly wants immediate action\n"
        "- For 'create and execute' requests, create TWO todos:\n"
        "  1. File creation todo (handled by file_tool)\n"
        "  2. Execution todo with exact command (handled by bash_tool)\n"
        "- Examples: 'create and run', 'build and test', 'make a script and execute it'\n"
        "- Use request_user_approval(reason) when:\n"
        "  * Operation might be risky or destructive\n"
        "  * User's intent is unclear or ambiguous\n"
        "  * Multiple approaches are possible\n"
        "  * You need more information before proceeding\n"
        "- NEVER use /do commands in responses (those are for CLI only)\n\n"
        
        "TODO FORMATS:\n"
        "1. Simple: add_todo(\"title\", \"description\") - for basic todos\n"
        "2. Separator format for complex content with quotes/special chars:\n"
        "   <TODO_SEPARATOR>\n"
        "   Title: Create complex Python script\n"
        "   Description: Script with embedded \"quotes\" and 'apostrophes' and newlines\n"
        "   Multi-line descriptions work perfectly!\n"
        "   <TODO_SEPARATOR>\n\n"
        
        "WORKFLOW:\n"
        "1. When users request file operations (create, write, modify files), add them to the todo list\n"
        "2. Break complex requests into specific todo items\n"
        "3. All todos are queued - the system will suggest when to execute them\n"
        "4. Users have full control over execution with /do commands\n\n"
        
        "RESPONSE FORMAT:\n"
        "- Be conversational and helpful\n"
        "- When adding todos, clearly state what you're adding (the system will automatically show **Added:** messages)\n"
        "- Simply mention that you've added the todo - don't use **Added:** formatting yourself\n"
        "- Let the system suggest execution - don't mention /do commands yourself\n\n"
        
        "CRITICAL: You MUST use the actual function calls in your response for them to work!\n\n"
        "EXAMPLES:\n"
        "User: 'create a hello world script'\n"
        "You: 'I'll create that for you! add_todo(\"Create hello world script\", \"Python script in src/ directory\") The todo has been added to your list. The system will suggest execution options.'\n\n"
        "User: 'create a counting script and execute it'\n"
        "You: 'I'll create and execute that for you! add_todo(\"Create counting script\", \"Python script that counts from 0 to 10\") add_todo(\"Execute counting script\", \"python3 src/counting.py\") execute_all() **Executed 2 pending todos** Done! The script has been created and executed.'\n\n"
        "User: 'I need tests and docs too'\n"
        "You: 'Great! add_todo(\"Create test files\", \"\") add_todo(\"Create documentation\", \"\") You now have multiple items queued. The system will suggest execution options.'\n\n"
        "User: 'add those and run the first one'\n"
        "You: 'add_todo(\"Setup project\", \"\") add_todo(\"Create main file\", \"\") I've added both todos. The system will suggest how to execute them.'\n\n"
        "User: 'how do I run my todos?'\n"
        "You: 'After creating todos, the system automatically suggests execution options. You can use /do next for one todo, /do all for everything, or check /todo to see your list.'\n\n"
        "User: 'delete all my old files and start fresh'\n"
        "You: 'I understand you want to start fresh, but I want to make sure this is safe. add_todo(\"Delete old files\", \"Remove existing project files\") request_user_approval(\"This will delete files - please confirm which files to remove\") **⏸️  Requesting User Approval:** This will delete files - please confirm which files to remove'\n\n"
        "User: 'create a Python script that prints Hello and asks for user name with quotes'\n"
        "You: 'I'll create a script with embedded quotes for you!\n"
        "<TODO_SEPARATOR>\n"
        "Title: Create interactive Python script\n"
        "Description: Script that prints \"Hello World!\" and asks \"What's your name?\"\n"
        "Should handle user input and display nicely formatted output\n"
        "<TODO_SEPARATOR>\n"
        "The todo is now queued with all the special characters handled safely!'\n\n"
        "User: 'show me my todos'\n"
        "You: 'Here are your current todos: list_todos()'";
    
    // Build request payload with chat system prompt
    nlohmann::json payload = provider_->build_request_payload(chat_system_prompt, user_prompt, model_);
    
    std::string url = provider_->get_full_url(api_key_, model_);
    std::string payload_str = payload.dump();
    
    // Get headers
    std::vector<std::string> headers = provider_->get_headers(api_key_);
    
    std::cout << "Making chat request to: " << url << std::endl;
    
    // Make HTTP request
    HttpResponse response = http_client_.post(url, payload_str, headers);
    
    if (!response.success) {
        throw std::runtime_error("HTTP request failed: " + response.error_message + 
                                " (Status: " + std::to_string(response.status_code) + ")");
    }
    
    // For chat mode, we need to extract the text response without trying to parse as WriteFileCommand
    return provider_->parse_chat_response(response.data);
}

std::string LLMClient::get_chat_response_with_history(const std::vector<ConversationMessage>& conversation_history) const {
    // Todo-centric chat system prompt with tool access
    std::string chat_system_prompt = 
        "You are MAG (Multi-Agent Gateway), a helpful AI assistant with todo management capabilities. "
        "You are currently in CHAT MODE where you can have natural conversations AND manage a todo list.\n\n";
        
    // Add policy constraints dynamically
    try {
        PolicyChecker policy;
        auto allowed_dirs = policy.get_allowed_directories("file_tool", "create");
        
        if (!allowed_dirs.empty()) {
            chat_system_prompt += "IMPORTANT POLICY CONSTRAINTS:\n";
            chat_system_prompt += "- When suggesting file operations, remember that files can ONLY be created in: ";
            for (size_t i = 0; i < allowed_dirs.size(); ++i) {
                chat_system_prompt += allowed_dirs[i];
                if (i < allowed_dirs.size() - 1) chat_system_prompt += ", ";
            }
            chat_system_prompt += "\n";
            chat_system_prompt += "- Files in other directories are NOT allowed\n";
            chat_system_prompt += "- If someone asks for files elsewhere, suggest alternatives in allowed directories\n\n";
        }
    } catch (const std::exception& e) {
        // If policy loading fails, fall back to basic constraints
        chat_system_prompt += "IMPORTANT POLICY CONSTRAINTS:\n"
                             "- Please suggest creating files in appropriate directories\n"
                             "- Avoid suggesting files in the project root\n\n";
    }
        
    chat_system_prompt += "AVAILABLE TOOLS:\n"
        "- add_todo(title, description): Add a new todo item (simple format)\n"
        "- TODO_START/TODO_END blocks: Add complex todos with quotes/special chars\n"
        "- list_todos(): Show current todos\n"
        "- update_todo(id, title, description): Modify existing todo\n"
        "- delete_todo(id): Remove todo item\n"
        "\nWhen creating todos, you can suggest BOTH file operations AND bash commands:\n"
        "- File operations: 'Create config.json with settings', 'Update README.md'\n"
        "- Bash commands: Use EXACT command syntax like 'python3 src/script.py', 'make clean', 'npm install'\n"
        "- For bash todos, be SPECIFIC with executable commands, not descriptions\n"
        "- Examples: 'python3 src/app.py' (not 'run the Python app'), 'ls -la' (not 'list files')\n"
        "- When creating script execution todos, use the exact command: 'python3 src/filename.py'\n"
        "- The system will automatically route each todo to the appropriate tool (file_tool or bash_tool)\n"
        "- mark_complete(id): Mark todo as done\n"
        "\nAUTONOMOUS EXECUTION TOOLS:\n"
        "- execute_next(): Execute the next pending todo autonomously\n"
        "- execute_all(): Execute all pending todos autonomously  \n"
        "- execute_todo(id): Execute a specific todo by ID\n"
        "- request_user_approval(reason): Stop and ask user for approval when uncertain\n\n"
        "EXECUTION COMMANDS (for the user):\n"
        "- /do: Execute all pending todos\n"
        "- /do next: Execute only the next pending todo\n"
        "- /do until N: Execute todos until (not including) ID N\n"
        "- /do N-M: Execute todos in range [start_id, end_id]\n"
        "- /do N: Execute specific todo by ID\n\n"
        
        "AUTONOMOUS EXECUTION GUIDELINES:\n"
        "- Use execute_next() or execute_all() when user clearly wants immediate action\n"
        "- For 'create and execute' requests, create TWO todos:\n"
        "  1. File creation todo (handled by file_tool)\n"
        "  2. Execution todo with exact command (handled by bash_tool)\n"
        "- Examples: 'create and run', 'build and test', 'make a script and execute it'\n"
        "- Use request_user_approval(reason) when:\n"
        "  * Operation might be risky or destructive\n"
        "  * User's intent is unclear or ambiguous\n"
        "  * Multiple approaches are possible\n"
        "  * You need more information before proceeding\n"
        "- NEVER use /do commands in responses (those are for CLI only)\n\n"
        
        "TODO FORMATS:\n"
        "1. Simple: add_todo(\"title\", \"description\") - for basic todos\n"
        "2. Separator format for complex content with quotes/special chars:\n"
        "   <TODO_SEPARATOR>\n"
        "   Title: Create complex Python script\n"
        "   Description: Script with embedded \"quotes\" and 'apostrophes' and newlines\n"
        "   Multi-line descriptions work perfectly!\n"
        "   <TODO_SEPARATOR>\n\n"
        
        "WORKFLOW:\n"
        "1. When users request file operations (create, write, modify files), add them to the todo list\n"
        "2. Break complex requests into specific todo items\n"
        "3. All todos are queued - the system will suggest when to execute them\n"
        "4. Users have full control over execution with /do commands\n\n"
        
        "RESPONSE FORMAT:\n"
        "- Be conversational and helpful\n"
        "- When adding todos, clearly state what you're adding (the system will automatically show **Added:** messages)\n"
        "- Simply mention that you've added the todo - don't use **Added:** formatting yourself\n"
        "- Let the system suggest execution - don't mention /do commands yourself\n\n"
        
        "CRITICAL: You MUST use the actual function calls in your response for them to work!\n\n";
    
    // Build request payload with conversation history and chat system prompt
    nlohmann::json payload = provider_->build_conversation_payload(chat_system_prompt, conversation_history, model_);
    
    std::string url = provider_->get_full_url(api_key_, model_);
    std::string payload_str = payload.dump();
    
    // Get headers
    std::vector<std::string> headers = provider_->get_headers(api_key_);
    
    std::cout << "Making chat request with conversation history to: " << url << std::endl;
    
    // Make HTTP request
    HttpResponse response = http_client_.post(url, payload_str, headers);
    
    if (!response.success) {
        throw std::runtime_error("HTTP request failed: " + response.error_message + 
                                " (Status: " + std::to_string(response.status_code) + ")");
    }
    
    // For chat mode, we need to extract the text response without trying to parse as WriteFileCommand
    return provider_->parse_chat_response(response.data);
}

} // namespace mag