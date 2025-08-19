#include "cli_interface.h"
#include "coordinator.h"
#include <iostream>
#include <string>

using namespace mag;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [PROMPT]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --provider=PROVIDER   Set LLM provider (gemini|chatgpt|claude|mistral)\n";
    std::cout << "  --help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << "                                    # Interactive CLI mode\n";
    std::cout << "  " << program_name << " \"Create hello.py\"                  # CLI mode with auto-detected provider\n";
    std::cout << "  " << program_name << " --provider=claude \"Create hello.py\" # CLI mode with specific provider\n\n";
    std::cout << "Interactive Mode Commands:\n";
    std::cout << "  /gemini    - Switch to Gemini provider\n";
    std::cout << "  /chatgpt   - Switch to ChatGPT provider\n";
    std::cout << "  /claude    - Switch to Claude provider\n";
    std::cout << "  /mistral   - Switch to Mistral provider\n";
    std::cout << "  /help      - Show help\n";
    std::cout << "  /exit      - Exit MAG\n";
}

int main(int argc, char* argv[]) {
    try {
        std::string provider_override;
        std::string user_prompt;
        bool interactive_mode = true;
        
        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                return 0;
            } else if (arg.find("--provider=") == 0) {
                provider_override = arg.substr(11); // Length of "--provider="
                // Validate provider
                if (provider_override != "gemini" && provider_override != "chatgpt" && 
                    provider_override != "claude" && provider_override != "mistral") {
                    std::cerr << "Error: Invalid provider '" << provider_override << "'" << std::endl;
                    std::cerr << "Valid providers: gemini, chatgpt, claude, mistral" << std::endl;
                    return 1;
                }
            } else {
                // This is part of the prompt
                if (!user_prompt.empty()) user_prompt += " ";
                user_prompt += arg;
                interactive_mode = false;
            }
        }
        
        if (interactive_mode) {
            // Interactive CLI mode
            CLIInterface interface(provider_override);
            interface.run();
        } else {
            // Command line mode
            if (user_prompt.empty()) {
                std::cerr << "Error: Empty prompt provided" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
            
            Coordinator coordinator(provider_override);
            coordinator.run(user_prompt);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}