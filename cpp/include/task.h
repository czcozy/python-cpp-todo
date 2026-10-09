#pragma once

#include <string>
#include <utility>

namespace todo {

// Use enum class rather than a bare int: you can only write Priority::High,
// never accidentally mix it with plain numbers.
enum class Priority {
  Low = 0,
  Medium = 1,  // default
  High = 2,
};

// enum class cannot be printed or saved directly, so write the conversions yourself.
std::string priority_to_text(Priority p);  // Priority -> "high/medium/low" (for the user)
int priority_to_number(Priority p);        // Priority -> 0/1/2 (for writing to a file)
Priority priority_from_number(int value);  // 0/1/2 -> Priority (for reading a file)

// struct and class differ only in default access; struct fits pure data better.
struct Task {
  int id = 0;  // assigned by TaskManager
  std::string title;
  Priority priority = Priority::Medium;
  bool done = false;
  std::string created_at;
  std::string due_date;  // like "2026-10-03" (day precision only)

  Task() = default;  // default constructor: Task t;

  // Construct a task (members are set in the initializer list; std::move avoids a string copy).
  Task(int id, std::string title, Priority priority, std::string created_at,
       std::string due_date)
      : id(id),
        title(std::move(title)),
        priority(priority),
        created_at(std::move(created_at)),
        due_date(std::move(due_date)) {}

  // Serialization: Task -> one line of text (the trailing const means it does not modify the object).
  std::string to_line() const;

  // Deserialization: one line of text -> Task (static, called as Task::from_line(...)).
  static Task from_line(const std::string& line);
};

// Current local time, formatted "2026-10-03 15:30".
std::string now_string();

}  // namespace todo
