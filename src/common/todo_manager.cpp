#include "todo_manager.h"
#include <algorithm>
#include <stdexcept>

namespace mag {

// TodoItem implementation
nlohmann::json TodoItem::to_json() const {
    auto now_time_t = std::chrono::system_clock::to_time_t(created_at);
    auto updated_time_t = std::chrono::system_clock::to_time_t(updated_at);
    
    return {
        {"id", id},
        {"title", title},
        {"description", description},
        {"status", status_to_string(status)},
        {"created_at", static_cast<long>(now_time_t)},
        {"updated_at", static_cast<long>(updated_time_t)}
    };
}

void TodoItem::from_json(const nlohmann::json& j) {
    id = j["id"];
    title = j["title"];
    description = j["description"];
    status = string_to_status(j["status"]);
    
    auto created_time_t = static_cast<std::time_t>(j["created_at"]);
    auto updated_time_t = static_cast<std::time_t>(j["updated_at"]);
    created_at = std::chrono::system_clock::from_time_t(created_time_t);
    updated_at = std::chrono::system_clock::from_time_t(updated_time_t);
}

// TodoManager implementation
TodoManager::TodoManager() : next_id_(1) {}

int TodoManager::add_todo(const std::string& title, const std::string& description) {
    if (title.empty()) {
        throw std::invalid_argument("Todo title cannot be empty");
    }
    
    TodoItem item;
    item.id = next_id_++;
    item.title = title;
    item.description = description;
    item.status = TodoStatus::PENDING;
    item.created_at = std::chrono::system_clock::now();
    item.updated_at = item.created_at;
    
    todos_.push_back(item);
    return item.id;
}

std::vector<TodoItem> TodoManager::list_todos(bool show_completed) const {
    if (show_completed) {
        return todos_;
    }
    
    std::vector<TodoItem> active_todos;
    std::copy_if(todos_.begin(), todos_.end(), std::back_inserter(active_todos),
                 [](const TodoItem& item) {
                     return item.status != TodoStatus::COMPLETED;
                 });
    return active_todos;
}

bool TodoManager::update_todo(int id, const std::string* title, 
                             const std::string* description, 
                             const TodoStatus* status) {
    auto it = find_todo(id);
    if (it == todos_.end()) {
        return false;
    }
    
    bool updated = false;
    if (title && !title->empty() && it->title != *title) {
        it->title = *title;
        updated = true;
    }
    
    if (description && it->description != *description) {
        it->description = *description;
        updated = true;
    }
    
    if (status && it->status != *status) {
        it->status = *status;
        updated = true;
    }
    
    if (updated) {
        update_timestamp(*it);
    }
    
    return updated;
}

bool TodoManager::delete_todo(int id) {
    auto it = find_todo(id);
    if (it == todos_.end()) {
        return false;
    }
    
    todos_.erase(it);
    return true;
}

void TodoManager::clear_todos() {
    todos_.clear();
}

TodoItem* TodoManager::get_todo(int id) {
    auto it = find_todo(id);
    return (it != todos_.end()) ? &(*it) : nullptr;
}

std::vector<TodoItem> TodoManager::get_pending_todos() const {
    std::vector<TodoItem> pending;
    std::copy_if(todos_.begin(), todos_.end(), std::back_inserter(pending),
                 [](const TodoItem& item) {
                     return item.status == TodoStatus::PENDING;
                 });
    return pending;
}

std::vector<TodoItem> TodoManager::get_completed_todos() const {
    std::vector<TodoItem> completed;
    std::copy_if(todos_.begin(), todos_.end(), std::back_inserter(completed),
                 [](const TodoItem& item) {
                     return item.status == TodoStatus::COMPLETED;
                 });
    return completed;
}

bool TodoManager::is_empty() const {
    return todos_.empty();
}

size_t TodoManager::count() const {
    return todos_.size();
}

size_t TodoManager::count_pending() const {
    return std::count_if(todos_.begin(), todos_.end(),
                        [](const TodoItem& item) {
                            return item.status == TodoStatus::PENDING;
                        });
}

bool TodoManager::mark_in_progress(int id) {
    TodoStatus status = TodoStatus::IN_PROGRESS;
    return update_todo(id, nullptr, nullptr, &status);
}

bool TodoManager::mark_completed(int id) {
    TodoStatus status = TodoStatus::COMPLETED;
    return update_todo(id, nullptr, nullptr, &status);
}

bool TodoManager::mark_pending(int id) {
    TodoStatus status = TodoStatus::PENDING;
    return update_todo(id, nullptr, nullptr, &status);
}

TodoItem* TodoManager::get_next_pending() {
    auto it = std::find_if(todos_.begin(), todos_.end(),
                          [](const TodoItem& item) {
                              return item.status == TodoStatus::PENDING;
                          });
    
    if (it != todos_.end()) {
        return &(*it);
    }
    return nullptr;
}

std::vector<TodoItem> TodoManager::get_execution_queue() const {
    auto pending = get_pending_todos();
    // Sort by creation time (FIFO execution)
    std::sort(pending.begin(), pending.end(),
              [](const TodoItem& a, const TodoItem& b) {
                  return a.created_at < b.created_at;
              });
    return pending;
}

std::vector<TodoItem> TodoManager::get_todos_until(int stop_id) const {
    std::vector<TodoItem> result;
    auto pending = get_execution_queue(); // Already sorted by creation time
    
    for (const auto& todo : pending) {
        if (todo.id == stop_id) {
            break; // Stop before including stop_id
        }
        result.push_back(todo);
    }
    
    return result;
}

std::vector<TodoItem> TodoManager::get_todos_range(int start_id, int end_id) const {
    std::vector<TodoItem> result;
    auto pending = get_execution_queue(); // Already sorted by creation time
    
    bool found_start = false;
    for (const auto& todo : pending) {
        if (todo.id == start_id) {
            found_start = true;
        }
        
        if (found_start) {
            result.push_back(todo);
            
            if (todo.id == end_id) {
                break; // Include end_id and stop
            }
        }
    }
    
    return result;
}

nlohmann::json TodoManager::to_json() const {
    nlohmann::json j;
    j["next_id"] = next_id_;
    j["todos"] = nlohmann::json::array();
    
    for (const auto& todo : todos_) {
        j["todos"].push_back(todo.to_json());
    }
    
    return j;
}

void TodoManager::from_json(const nlohmann::json& j) {
    next_id_ = j["next_id"];
    todos_.clear();
    
    for (const auto& todo_json : j["todos"]) {
        TodoItem item;
        item.from_json(todo_json);
        todos_.push_back(item);
    }
}

void TodoManager::update_timestamp(TodoItem& item) {
    item.updated_at = std::chrono::system_clock::now();
}

std::vector<TodoItem>::iterator TodoManager::find_todo(int id) {
    return std::find_if(todos_.begin(), todos_.end(),
                       [id](const TodoItem& item) {
                           return item.id == id;
                       });
}

std::vector<TodoItem>::const_iterator TodoManager::find_todo(int id) const {
    return std::find_if(todos_.begin(), todos_.end(),
                       [id](const TodoItem& item) {
                           return item.id == id;
                       });
}

// Utility functions
std::string status_to_string(TodoStatus status) {
    switch (status) {
        case TodoStatus::PENDING: return "pending";
        case TodoStatus::IN_PROGRESS: return "in_progress";
        case TodoStatus::COMPLETED: return "completed";
        default: return "unknown";
    }
}

TodoStatus string_to_status(const std::string& status_str) {
    if (status_str == "pending") return TodoStatus::PENDING;
    if (status_str == "in_progress") return TodoStatus::IN_PROGRESS;
    if (status_str == "completed") return TodoStatus::COMPLETED;
    throw std::invalid_argument("Unknown status: " + status_str);
}

} // namespace mag