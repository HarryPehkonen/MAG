#include "message.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace mag {

// GenericCommand implementations
void GenericCommand::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"type", static_cast<int>(type)},
        {"description", description}
    };
    
    if (type == OperationType::FILE_WRITE) {
        j["file_path"] = file_path;
        j["file_content"] = file_content;
    } else if (type == OperationType::BASH_COMMAND) {
        j["bash_command"] = bash_command;
        j["working_directory"] = working_directory;
    }
}

void GenericCommand::from_json(const nlohmann::json& j) {
    type = static_cast<OperationType>(j.at("type").get<int>());
    j.at("description").get_to(description);
    
    if (type == OperationType::FILE_WRITE) {
        if (j.contains("file_path")) j.at("file_path").get_to(file_path);
        if (j.contains("file_content")) j.at("file_content").get_to(file_content);
    } else if (type == OperationType::BASH_COMMAND) {
        if (j.contains("bash_command")) j.at("bash_command").get_to(bash_command);
        if (j.contains("working_directory")) j.at("working_directory").get_to(working_directory);
    }
}

WriteFileCommand GenericCommand::to_write_file_command() const {
    WriteFileCommand cmd;
    if (type == OperationType::FILE_WRITE) {
        cmd.command = "write";
        cmd.path = file_path;
        cmd.content = file_content;
    } else {
        throw std::runtime_error("Cannot convert non-file command to WriteFileCommand");
    }
    return cmd;
}

std::string GenericCommand::get_operation_summary() const {
    if (type == OperationType::FILE_WRITE) {
        return "WriteFile " + file_path;
    } else if (type == OperationType::BASH_COMMAND) {
        return "BashCommand: " + bash_command;
    }
    return "Unknown operation";
}

// ExecutionContext implementations
void ExecutionContext::to_json(nlohmann::json& j) const {
    auto duration = timestamp.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    j = nlohmann::json{
        {"working_directory_before", working_directory_before},
        {"working_directory_after", working_directory_after},
        {"command_output", command_output},
        {"command_stderr", command_stderr},
        {"exit_code", exit_code},
        {"timestamp_ms", millis}
    };
}

void ExecutionContext::from_json(const nlohmann::json& j) {
    if (j.contains("working_directory_before")) {
        j.at("working_directory_before").get_to(working_directory_before);
    }
    if (j.contains("working_directory_after")) {
        j.at("working_directory_after").get_to(working_directory_after);
    }
    if (j.contains("command_output")) {
        j.at("command_output").get_to(command_output);
    }
    if (j.contains("command_stderr")) {
        j.at("command_stderr").get_to(command_stderr);
    }
    if (j.contains("exit_code")) {
        j.at("exit_code").get_to(exit_code);
    }
    if (j.contains("timestamp_ms")) {
        auto millis = j.at("timestamp_ms").get<long long>();
        timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(millis));
    }
}

bool ExecutionContext::has_output() const {
    return !command_output.empty() || !command_stderr.empty();
}

std::string ExecutionContext::get_combined_output() const {
    std::string combined = command_output;
    if (!command_stderr.empty()) {
        if (!combined.empty()) combined += "\n";
        combined += "[STDERR]: " + command_stderr;
    }
    return combined;
}

std::string ExecutionContext::to_summary_string() const {
    std::ostringstream oss;
    oss << "Context: ";
    if (!working_directory_after.empty()) {
        oss << "pwd=" << working_directory_after;
    }
    if (exit_code != 0) {
        oss << " exit_code=" << exit_code;
    }
    if (has_output()) {
        oss << " [has_output]";
    }
    return oss.str();
}

void WriteFileCommand::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"command", command},
        {"path", path},
        {"content", content},
        {"request_execution", request_execution}
    };
}

void WriteFileCommand::from_json(const nlohmann::json& j) {
    j.at("command").get_to(command);
    j.at("path").get_to(path);
    j.at("content").get_to(content);
    if (j.contains("request_execution")) {
        j.at("request_execution").get_to(request_execution);
    }
}

// BashCommand implementations
void BashCommand::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"command", command},
        {"bash_command", bash_command},
        {"working_directory", working_directory},
        {"description", description},
        {"request_execution", request_execution}
    };
}

void BashCommand::from_json(const nlohmann::json& j) {
    j.at("command").get_to(command);
    j.at("bash_command").get_to(bash_command);
    if (j.contains("working_directory")) {
        j.at("working_directory").get_to(working_directory);
    }
    if (j.contains("description")) {
        j.at("description").get_to(description);
    }
    if (j.contains("request_execution")) {
        j.at("request_execution").get_to(request_execution);
    }
}

std::string BashCommand::get_summary() const {
    std::string summary = "Bash: " + bash_command;
    if (!working_directory.empty()) {
        summary += " (in " + working_directory + ")";
    }
    return summary;
}

void DryRunResult::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"description", description},
        {"success", success},
        {"error_message", error_message}
    };
}

void DryRunResult::from_json(const nlohmann::json& j) {
    j.at("description").get_to(description);
    j.at("success").get_to(success);
    if (j.contains("error_message")) {
        j.at("error_message").get_to(error_message);
    }
}

void ApplyResult::to_json(nlohmann::json& j) const {
    j = nlohmann::json{
        {"description", description},
        {"success", success},
        {"error_message", error_message}
    };
    
    // Add execution context
    nlohmann::json context_json;
    execution_context.to_json(context_json);
    j["execution_context"] = context_json;
}

void ApplyResult::from_json(const nlohmann::json& j) {
    j.at("description").get_to(description);
    j.at("success").get_to(success);
    if (j.contains("error_message")) {
        j.at("error_message").get_to(error_message);
    }
    
    // Load execution context if present
    if (j.contains("execution_context")) {
        execution_context.from_json(j.at("execution_context"));
    }
}

std::string ApplyResult::get_execution_summary() const {
    std::ostringstream oss;
    oss << description;
    if (!execution_context.working_directory_after.empty()) {
        oss << " | " << execution_context.to_summary_string();
    }
    return oss.str();
}

bool ApplyResult::has_context_output() const {
    return execution_context.has_output();
}

std::string MessageHandler::serialize_command(const WriteFileCommand& cmd) {
    nlohmann::json j;
    cmd.to_json(j);
    return j.dump();
}

WriteFileCommand MessageHandler::deserialize_command(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    WriteFileCommand cmd;
    cmd.from_json(j);
    return cmd;
}

std::string MessageHandler::serialize_dry_run_result(const DryRunResult& result) {
    nlohmann::json j;
    result.to_json(j);
    return j.dump();
}

DryRunResult MessageHandler::deserialize_dry_run_result(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    DryRunResult result;
    result.from_json(j);
    return result;
}

std::string MessageHandler::serialize_apply_result(const ApplyResult& result) {
    nlohmann::json j;
    result.to_json(j);
    return j.dump();
}

ApplyResult MessageHandler::deserialize_apply_result(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    ApplyResult result;
    result.from_json(j);
    return result;
}

std::string MessageHandler::serialize_execution_context(const ExecutionContext& context) {
    nlohmann::json j;
    context.to_json(j);
    return j.dump();
}

ExecutionContext MessageHandler::deserialize_execution_context(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    ExecutionContext context;
    context.from_json(j);
    return context;
}

std::string MessageHandler::serialize_bash_command(const BashCommand& cmd) {
    nlohmann::json j;
    cmd.to_json(j);
    return j.dump();
}

BashCommand MessageHandler::deserialize_bash_command(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    BashCommand cmd;
    cmd.from_json(j);
    return cmd;
}

} // namespace mag