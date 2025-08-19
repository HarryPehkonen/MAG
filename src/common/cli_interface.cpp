#include "cli_interface.h"
#include "utils.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

namespace mag {

CLIInterface::CLIInterface() 
    : coordinator_(), running_(true) {
    input_handler_ = create_input_handler();
    conversation_manager_ = std::make_unique<ConversationManager>();
    init_debug_log();
    setup_completion();
    debug_log_ << "[CLI] CLIInterface initialized with conversation persistence" << std::endl;
}

CLIInterface::CLIInterface(const std::string& provider_override) 
    : coordinator_(provider_override), running_(true) {
    input_handler_ = create_input_handler();
    conversation_manager_ = std::make_unique<ConversationManager>();
    init_debug_log();
    setup_completion();
    debug_log_ << "[CLI] CLIInterface initialized with provider: " << provider_override << " and conversation persistence" << std::endl;
}

CLIInterface::~CLIInterface() {
    if (debug_log_.is_open()) {
        debug_log_ << "[CLI] CLIInterface destroyed" << std::endl;
        debug_log_.close();
    }
}

void CLIInterface::run() {
    show_welcome();
    
    debug_log_ << "[CLI] Starting main command loop" << std::endl;
    
    while (running_) {
        std::string input = input_handler_->get_line(get_prompt());
        
        // Handle EOF (Ctrl+D)
        if (input.empty()) {
            std::cout << "\nGoodbye!\n";
            break;
        }
        
        // Skip empty lines
        if (input.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        // Add to history
        input_handler_->add_history(input);
        
        // Handle the command
        handle_command(input);
    }
    
    debug_log_ << "[CLI] Main command loop ended" << std::endl;
}

void CLIInterface::handle_command(const std::string& input) {
    debug_log_ << "[CLI] Handling command: " << input << std::endl;
    
    try {
        // Check for slash commands (must start at beginning of line, no leading whitespace)
        if (!input.empty() && input[0] == '/') {
            debug_log_ << "[CLI] Detected slash command: " << input << std::endl;
            handle_slash_command(input.substr(1));
            return;
        }
        
        // Regular chat/file request
        print_colored("Processing: " + input, "36"); // Cyan
        std::cout << std::endl;
        
        // Add user message to conversation history
        conversation_manager_->add_user_message(input);
        
        debug_log_ << "[CLI] Calling coordinator with conversation history (" 
                  << conversation_manager_->get_message_count() << " messages)" << std::endl;
        
        // Use conversation history for better context
        auto history = conversation_manager_->get_history();
        std::string response = coordinator_.run_with_conversation_history(input, history);
        
        // Save assistant response to conversation history if we got one
        if (!response.empty() && response.find("Error:") != 0) {
            // Get current provider from coordinator
            std::string current_provider = coordinator_.get_current_provider();
            conversation_manager_->add_assistant_message(response, current_provider);
        }
        
        debug_log_ << "[CLI] coordinator.run_with_conversation_history() completed with response length: " 
                  << response.length() << std::endl;
        
    } catch (const std::exception& e) {
        debug_log_ << "[CLI] Exception in handle_command: " << e.what() << std::endl;
        print_colored("Error: " + std::string(e.what()), "31"); // Red
        std::cout << std::endl;
    }
}

void CLIInterface::handle_slash_command(const std::string& command) {
    debug_log_ << "[CLI] Handling slash command: " << command << std::endl;
    
    if (command == "help" || command == "h") {
        show_help();
    } else if (command == "status") {
        show_status();
    } else if (command == "debug") {
        show_debug();
    } else if (command == "exit" || command == "quit" || command == "q") {
        running_ = false;
    } else if (command == "gemini" || command == "claude" || command == "chatgpt" || command == "mistral") {
        switch_provider_with_context(command);
    } else if (command == "todo") {
        show_todo_list();
    } else if (command.substr(0, 2) == "do") {
        handle_do_command(command);
    } else if (command == "pause") {
        coordinator_.pause_execution();
    } else if (command == "resume") {
        coordinator_.resume_execution();
    } else if (command == "stop") {
        coordinator_.stop_execution();
    } else if (command == "cancel") {
        coordinator_.cancel_execution();
    } else if (command == "status") {
        show_execution_status();
    } else if (command == "history") {
        show_conversation_history();
    } else if (command.substr(0, 7) == "session") {
        handle_session_command(command.substr(7));
    } else {
        print_colored("Unknown command: /" + command, "33"); // Yellow
        std::cout << "\nType '/help' for available commands.\n";
    }
}

void CLIInterface::show_welcome() {
    print_colored("MAG v1.0.0 - Multi-Agent Gateway", "34"); // Blue
    std::cout << std::endl;
    print_colored("Chat mode enabled with todo tool integration", "32"); // Green
    std::cout << std::endl;
    
    if (input_handler_->supports_advanced_features()) {
        std::cout << "Enhanced CLI with command history and tab completion enabled.\n";
    }
    
    std::cout << "Type '/help' for commands, '/exit' to quit.\n";
    std::cout << std::endl;
}

void CLIInterface::show_help() {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  /gemini, /claude, /chatgpt, /mistral  - Switch LLM provider\n";
    std::cout << "  /debug                                - Show debug information\n";
    std::cout << "  /todo                                 - Show todo list\n";
    std::cout << "  /do [all|next|until N|N-M]           - Execute todos\n";
    std::cout << "  /pause                                - Pause execution\n";
    std::cout << "  /resume                               - Resume paused execution\n";
    std::cout << "  /stop                                 - Stop execution\n";
    std::cout << "  /cancel                               - Cancel execution\n";
    std::cout << "  /status                               - Show execution status\n";
    std::cout << "  /help, /h                             - Show this help\n";
    std::cout << "  /exit, /quit, /q                      - Exit MAG\n";
    std::cout << "\nOr just type your request naturally:\n";
    std::cout << "  \"create a hello world Python script\"\n";
    std::cout << "  \"help me refactor this code\"\n";
    std::cout << "  \"add unit tests for the calculator\"\n";
    std::cout << std::endl;
}

void CLIInterface::show_status() {
    std::cout << "\n=== MAG System Status ===\n";
    std::cout << "Mode: Chat with todo tool integration\n";
    std::cout << "Input: " << (input_handler_->supports_advanced_features() ? "Readline (enhanced)" : "Simple") << "\n";
    std::cout << "Debug log: .mag/debug.log\n";
    std::cout << "History: .mag/history\n";
    std::cout << "Policy: .mag/policy.json\n";
    
    // Check if services are running by checking if we can connect
    std::cout << "Services: LLM adapter + File tool (check with '/debug' if issues)\n";
    std::cout << std::endl;
}

void CLIInterface::show_debug() {
    std::cout << "\n=== Debug Information ===\n";
    std::cout << "Debug log: .mag/debug.log\n";
    std::cout << "Policy file: .mag/policy.json\n";
    std::cout << "History file: .mag/history\n";
    std::cout << "Features: " << (input_handler_->supports_advanced_features() ? "Advanced" : "Basic") << "\n";
    
    // Show recent debug log entries
    std::cout << "\nRecent debug log entries:\n";
    std::system("tail -5 .mag/debug.log 2>/dev/null || echo 'No debug log found'");
    std::cout << std::endl;
}

void CLIInterface::init_debug_log() {
    // Create .mag directory if it doesn't exist
    std::string mag_dir = Utils::get_current_working_directory() + "/.mag";
    Utils::create_directories(mag_dir);
    
    debug_log_.open(mag_dir + "/debug.log", std::ios::app);
    if (debug_log_.is_open()) {
        debug_log_ << "\n=== MAG CLI Debug Log Session Started ===\n";
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        debug_log_ << "Timestamp: " << std::ctime(&time_t);
    }
}

void CLIInterface::setup_completion() {
    std::vector<std::string> completions = {
        "/help", "/h",
        "/status",
        "/debug", 
        "/todo",
        "/do", "/do all", "/do next",
        "/exit", "/quit", "/q",
        "/gemini", "/claude", "/chatgpt", "/mistral"
    };
    
    input_handler_->setup_completion(completions);
}

std::string CLIInterface::get_prompt() const {
    if (supports_colors()) {
        return "\033[1;35mMAG>\033[0m "; // Bold magenta
    } else {
        return "MAG> ";
    }
}

void CLIInterface::print_colored(const std::string& text, const std::string& color) {
    if (supports_colors() && !color.empty()) {
        std::cout << "\033[" << color << "m" << text << "\033[0m";
    } else {
        std::cout << text;
    }
}

bool CLIInterface::supports_colors() const {
    // Check if we're running in a terminal that supports colors
    const char* term = std::getenv("TERM");
    return term != nullptr && std::string(term) != "dumb";
}

void CLIInterface::show_todo_list() {
    auto todos = coordinator_.get_todo_manager().list_todos(true);
    
    std::cout << "\n=== Todo List ===\n";
    if (todos.empty()) {
        std::cout << "No todos yet.\n";
    } else {
        for (const auto& todo : todos) {
            std::string status_icon;
            std::string color;
            switch (todo.status) {
                case TodoStatus::PENDING:
                    status_icon = "⏳";
                    color = "33"; // Yellow
                    break;
                case TodoStatus::IN_PROGRESS:
                    status_icon = "🔄";
                    color = "36"; // Cyan
                    break;
                case TodoStatus::COMPLETED:
                    status_icon = "✅";
                    color = "32"; // Green
                    break;
            }
            
            print_colored(status_icon + " " + std::to_string(todo.id) + ": " + todo.title, color);
            std::cout << std::endl;
            if (!todo.description.empty()) {
                std::cout << "   " << todo.description << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

void CLIInterface::show_execution_status() {
    auto state = coordinator_.get_execution_state();
    
    std::cout << "\n=== Execution Status ===\n";
    
    switch (state) {
        case Coordinator::ExecutionState::STOPPED:
            print_colored("Status: STOPPED", "37"); // White
            std::cout << "\nUse /do to start running todos\n";
            break;
        case Coordinator::ExecutionState::RUNNING:
            print_colored("Status: RUNNING", "32"); // Green
            std::cout << "\nExecution in progress...\n";
            std::cout << "Use /pause, /stop, or /cancel to control\n";
            break;
        case Coordinator::ExecutionState::PAUSED:
            print_colored("Status: PAUSED", "33"); // Yellow
            std::cout << "\nExecution paused\n";
            std::cout << "Use /resume to continue or /stop to stop\n";
            break;
        case Coordinator::ExecutionState::CANCELLED:
            print_colored("Status: CANCELLED", "31"); // Red
            std::cout << "\nLast execution was cancelled\n";
            std::cout << "Use /do to start new execution\n";
            break;
    }
    std::cout << std::endl;
}

void CLIInterface::handle_do_command(const std::string& command) {
    // Parse do command: /do [all|next|until N|N-M]
    std::string args = command.substr(2); // Remove "do"
    
    // Trim leading whitespace
    size_t start = args.find_first_not_of(" \t");
    if (start != std::string::npos) {
        args = args.substr(start);
    } else {
        args = "";
    }
    
    debug_log_ << "[CLI] Do command args: '" << args << "'" << std::endl;
    
    try {
        if (args.empty() || args == "all") {
            coordinator_.execute_todos();
        } else if (args == "next") {
            coordinator_.execute_next_todo();
        } else if (args.substr(0, 5) == "until") {
            std::string id_str = args.substr(5);
            // Trim whitespace
            start = id_str.find_first_not_of(" \t");
            if (start != std::string::npos) {
                id_str = id_str.substr(start);
                int stop_id = std::stoi(id_str);
                coordinator_.execute_todos_until(stop_id);
            } else {
                print_colored("Usage: /do until <id>", "33");
                std::cout << std::endl;
            }
        } else if (args.find('-') != std::string::npos) {
            // Range format: N-M
            size_t dash_pos = args.find('-');
            std::string start_str = args.substr(0, dash_pos);
            std::string end_str = args.substr(dash_pos + 1);
            
            int start_id = std::stoi(start_str);
            int end_id = std::stoi(end_str);
            coordinator_.execute_todos_range(start_id, end_id);
        } else {
            // Try to parse as single ID (execute specific todo)
            int todo_id = std::stoi(args);
            auto* todo = coordinator_.get_todo_manager().get_todo(todo_id);
            if (todo && todo->status == TodoStatus::PENDING) {
                coordinator_.get_todo_manager().mark_in_progress(todo_id);
                coordinator_.execute_single_todo(*todo);
                coordinator_.get_todo_manager().mark_completed(todo_id);
                print_colored("✅ Completed: " + todo->title, "32");
                std::cout << std::endl;
            } else {
                print_colored("Todo ID " + std::to_string(todo_id) + " not found or not pending.", "31");
                std::cout << std::endl;
            }
        }
    } catch (const std::exception& e) {
        print_colored("Do error: " + std::string(e.what()), "31");
        std::cout << std::endl;
        print_colored("Usage: /do [all|next|until <id>|<start>-<end>|<id>]", "33");
        std::cout << std::endl;
    }
}

void CLIInterface::switch_provider_with_context(const std::string& provider_name) {
    debug_log_ << "[CLI] Switching provider to: " << provider_name << " with conversation context" << std::endl;
    
    try {
        // Save current conversation state
        conversation_manager_->save_to_disk();
        
        // Switch provider
        coordinator_.set_provider(provider_name);
        
        // Show success with conversation context info
        print_colored("Switched to provider: " + provider_name, "32"); // Green
        
        if (!conversation_manager_->is_empty()) {
            std::cout << " (maintaining conversation context with " 
                      << conversation_manager_->get_message_count() << " messages)";
        }
        std::cout << std::endl;
        
        debug_log_ << "[CLI] Provider switched successfully with " 
                   << conversation_manager_->get_message_count() << " messages in context" << std::endl;
                   
    } catch (const std::exception& e) {
        print_colored("Error switching provider: " + std::string(e.what()), "31"); // Red
        std::cout << std::endl;
        debug_log_ << "[CLI] Error switching provider: " << e.what() << std::endl;
    }
}

void CLIInterface::show_conversation_history() {
    try {
        auto history = conversation_manager_->get_history();
        
        if (history.empty()) {
            print_colored("No conversation history available.", "33"); // Yellow
            std::cout << std::endl;
            return;
        }
        
        print_colored("=== Conversation History ===", "34"); // Blue
        std::cout << " (Session: " << conversation_manager_->get_current_session_id() << ")" << std::endl;
        
        for (size_t i = 0; i < history.size(); ++i) {
            const auto& msg = history[i];
            
            // Format role with color
            if (msg.role == "user") {
                print_colored("User", "36"); // Cyan
            } else if (msg.role == "assistant") {
                print_colored("Assistant", "32"); // Green
                if (!msg.provider.empty()) {
                    std::cout << " (" << msg.provider << ")";
                }
            } else if (msg.role == "system") {
                print_colored("System", "35"); // Magenta
            }
            
            std::cout << ": " << msg.content << std::endl;
            
            // Show timestamp for recent messages or if requested
            if (i >= history.size() - 5 || history.size() <= 10) {
                std::cout << "  " << msg.timestamp << std::endl;
            }
            std::cout << std::endl;
        }
        
        std::cout << "Total messages: " << history.size() << std::endl;
        
    } catch (const std::exception& e) {
        print_colored("Error showing conversation history: " + std::string(e.what()), "31");
        std::cout << std::endl;
    }
}

void CLIInterface::handle_session_command(const std::string& command) {
    debug_log_ << "[CLI] Handling session command: " << command << std::endl;
    
    try {
        std::string trimmed_command = command;
        // Remove leading spaces
        size_t start = trimmed_command.find_first_not_of(" ");
        if (start != std::string::npos) {
            trimmed_command = trimmed_command.substr(start);
        }
        
        if (trimmed_command.empty() || trimmed_command == " list") {
            // List available sessions
            auto sessions = conversation_manager_->get_available_sessions();
            
            print_colored("=== Available Conversation Sessions ===", "34"); // Blue
            std::cout << std::endl;
            
            if (sessions.empty()) {
                print_colored("No saved sessions found.", "33"); // Yellow
                std::cout << std::endl;
            } else {
                for (size_t i = 0; i < sessions.size() && i < 10; ++i) {
                    const auto& session = sessions[i];
                    std::cout << "  " << (i + 1) << ". " << session;
                    
                    if (session == conversation_manager_->get_current_session_id()) {
                        print_colored(" (current)", "32"); // Green
                    }
                    std::cout << std::endl;
                }
                
                if (sessions.size() > 10) {
                    std::cout << "  ... and " << (sessions.size() - 10) << " more" << std::endl;
                }
            }
            
        } else if (trimmed_command == " new") {
            // Start new session
            conversation_manager_->start_new_session();
            print_colored("Started new conversation session: " + 
                         conversation_manager_->get_current_session_id(), "32");
            std::cout << std::endl;
            
        } else if (trimmed_command.substr(0, 5) == " load") {
            // Load specific session
            std::string session_id = trimmed_command.substr(5);
            // Remove leading spaces
            size_t start = session_id.find_first_not_of(" ");
            if (start != std::string::npos) {
                session_id = session_id.substr(start);
            }
            
            if (session_id.empty()) {
                print_colored("Usage: /session load <session_id>", "33");
                std::cout << std::endl;
            } else if (conversation_manager_->load_session(session_id)) {
                print_colored("Loaded session: " + session_id + 
                             " (" + std::to_string(conversation_manager_->get_message_count()) + 
                             " messages)", "32");
                std::cout << std::endl;
            } else {
                print_colored("Failed to load session: " + session_id, "31");
                std::cout << std::endl;
            }
            
        } else {
            print_colored("Unknown session command. Usage:", "33");
            std::cout << std::endl;
            std::cout << "  /session       - List available sessions" << std::endl;
            std::cout << "  /session new   - Start new session" << std::endl;
            std::cout << "  /session load <id> - Load specific session" << std::endl;
        }
        
    } catch (const std::exception& e) {
        print_colored("Session error: " + std::string(e.what()), "31");
        std::cout << std::endl;
    }
}

} // namespace mag