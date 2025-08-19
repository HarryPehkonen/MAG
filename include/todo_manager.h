#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace mag {

enum class TodoStatus {
    PENDING,
    IN_PROGRESS, 
    COMPLETED
};

struct TodoItem {
    int id;
    std::string title;
    std::string description;
    TodoStatus status;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    // Serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

class TodoManager {
public:
    TodoManager();
    
    // Core CRUD operations
    int add_todo(const std::string& title, const std::string& description = "");
    std::vector<TodoItem> list_todos(bool show_completed = false) const;
    bool update_todo(int id, const std::string* title = nullptr, 
                     const std::string* description = nullptr, 
                     const TodoStatus* status = nullptr);
    bool delete_todo(int id);
    void clear_todos();
    
    // Query operations
    TodoItem* get_todo(int id);
    std::vector<TodoItem> get_pending_todos() const;
    std::vector<TodoItem> get_completed_todos() const;
    bool is_empty() const;
    size_t count() const;
    size_t count_pending() const;
    
    // Status operations
    bool mark_in_progress(int id);
    bool mark_completed(int id);
    bool mark_pending(int id);
    
    // Execution planning and control  
    std::vector<TodoItem> get_execution_queue() const; // pending todos in order
    TodoItem* get_next_pending(); // Get next todo to execute (or nullptr)
    std::vector<TodoItem> get_todos_until(int stop_id) const; // Get todos from start until (not including) stop_id
    std::vector<TodoItem> get_todos_range(int start_id, int end_id) const; // Get todos in range [start_id, end_id]
    
    // Serialization for persistence
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
private:
    std::vector<TodoItem> todos_;
    int next_id_;
    
    void update_timestamp(TodoItem& item);
    std::vector<TodoItem>::iterator find_todo(int id);
    std::vector<TodoItem>::const_iterator find_todo(int id) const;
};

// Utility functions
std::string status_to_string(TodoStatus status);
TodoStatus string_to_status(const std::string& status_str);

} // namespace mag