#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "storage.h"
#include "task.h"

namespace todo {

// Business layer: holds all tasks and handles CRUD.
class TaskManager {
 public:
  // Takes over a Storage; all later reads and writes go through it.
  explicit TaskManager(Storage storage);

  // Read data from disk and work out the next free id.
  void load();

  // Return all tasks, sorted by id from smallest to largest.
  std::vector<Task> all() const;

  // Return all tasks that are not done.
  std::vector<Task> pending() const;

  // Return the number of completed tasks.
  std::size_t count_done() const;

  // Find a task by id; returns an empty optional when not found (safer than a null pointer).
  std::optional<Task> find(int id) const;

  // Create a task and save it immediately, returning the new task's id.
  int add(const std::string& title, Priority priority = Priority::Medium,
          const std::string& due_date = "");

  // Find tasks by keyword in the title and return the matches.
  std::vector<Task> search(const std::string&) const;

  // Mark the task with the given id as done; returns true on success.
  bool complete(int id);

  // Delete the task with the given id; returns true on success.
  bool remove(int id);

  // Undo the last deletion, returning the restored task; empty optional if nothing to undo.
  std::optional<Task> undo_remove();

 private:
  std::vector<Task> tasks_;
  Storage storage_;
  int next_id_ = 1;

  // The most recently deleted task, used to support undo; empty when nothing can be undone.
  // Ids only ever increase and are never reused, so restoring can safely keep its old id.
  std::optional<Task> last_deleted_;

  // Save immediately after every change; not exposed publicly.
  void save() const;
};

}  // namespace todo
