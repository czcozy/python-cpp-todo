#include "manager.h"

#include <algorithm>

namespace todo {

TaskManager::TaskManager(Storage storage) : storage_(std::move(storage)) {}

void TaskManager::load() {
  tasks_ = storage_.load();

  // Use "current max id + 1"; the task count would repeat ids after deletions.
  next_id_ = 1;
  for (const Task& task : tasks_) {
    next_id_ = std::max(next_id_, task.id + 1);
  }
}

std::vector<Task> TaskManager::all() const {
  // Copy first and then sort, so whatever the caller does cannot affect the internal data.
  std::vector<Task> result = tasks_;
  std::sort(result.begin(), result.end(),
            [](const Task& a, const Task& b) { return a.id < b.id; });
  return result;
}

std::vector<Task> TaskManager::pending() const {
  std::vector<Task> result;
  // Iterate read-only with const Task& to avoid copying every task.
  for (const Task& task : tasks_) {
    if (!task.done) {
      result.push_back(task);
    }
  }

  // Sort by priority
  std::sort(result.begin(), result.end(), [](const Task& a, const Task& b) {
    return static_cast<int>(a.priority) > static_cast<int>(b.priority);
  });
  return result;
}

std::size_t TaskManager::count_done() const {
  // count_if: count the elements that satisfy a condition.
  return static_cast<std::size_t>(std::count_if(
      tasks_.begin(), tasks_.end(), [](const Task& t) { return t.done; }));
}

std::optional<Task> TaskManager::find(int id) const {
  for (const Task& task : tasks_) {
    if (task.id == id) {
      return task;
    }
  }
  return std::nullopt;
}

int TaskManager::add(const std::string& title, Priority priority,
                     const std::string& due_date) {
  Task task(next_id_, title, priority, now_string(), due_date);
  tasks_.push_back(task);
  ++next_id_;
  save();
  return task.id;
}

std::vector<Task> TaskManager::search(const std::string& keyword) const {
  std::vector<Task> result;
  for (const Task& task : tasks_) {
    if (task.title.find(keyword) != std::string::npos) {
      result.push_back(task);
    }
  }
  return result;
}

bool TaskManager::complete(int id) {
  // Iterate by reference (Task& instead of const Task&) to be able to modify elements.
  for (Task& task : tasks_) {
    if (task.id == id) {
      task.done = true;
      save();
      return true;
    }
  }
  return false;
}

bool TaskManager::remove(int id) {
  // Copy the task to delete first: after erase it is gone, and after remove_if the element
  // left at new_end is only a moved-from one (its contents are no longer valid), so it cannot
  // serve as the copy.
  const auto target = std::find_if(tasks_.begin(), tasks_.end(),
                                   [id](const Task& t) { return t.id == id; });
  if (target == tasks_.end()) {
    return false;  // not found, nothing changed
  }
  last_deleted_ = *target;

  // erase-remove: move the element to delete to the end, then erase in one go.
  const auto new_end = std::remove_if(
      tasks_.begin(), tasks_.end(), [id](const Task& t) { return t.id == id; });
  tasks_.erase(new_end, tasks_.end());
  save();
  return true;
}

std::optional<Task> TaskManager::undo_remove() {
  if (!last_deleted_) {
    return std::nullopt;  // nothing was deleted, so there is nothing to undo
  }
  // Put the task straight back, reusing its original id; ordering is left to all().
  const Task restored = *last_deleted_;
  tasks_.push_back(restored);
  // Only one undo is allowed: clear it afterwards so repeated undos cannot restore duplicates.
  last_deleted_.reset();
  save();
  return restored;
}

void TaskManager::save() const { storage_.save(tasks_); }

}  // namespace todo
