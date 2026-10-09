// Program entry point: only talks to the user; the logic lives in TaskManager.

#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "manager.h"
#include "storage.h"
#include "task.h"

// The default data file path is injected by CMake at compile time via -D (see CMakeLists.txt);
// a manual g++ build has no such macro and falls back to the relative path below.
#ifndef TODO_DATA_FILE
#define TODO_DATA_FILE "data/tasks.json"
#endif

// Use using declarations rather than using namespace std, to avoid name pollution.
using todo::Priority;
using todo::Task;
using todo::TaskManager;

namespace {

// Read one line of input and convert it to an integer, falling back when it cannot be parsed.
// Returning nullopt means the input stream ended (e.g. Ctrl+D); the caller exits on that,
// avoiding an infinite loop.
std::optional<int> read_int(const std::string& prompt, int fallback) {
  std::cout << prompt;
  std::string line;
  if (!std::getline(std::cin, line)) {
    return std::nullopt;  // input stream ended; fallback must not pretend this is valid input
  }
  try {
    return std::stoi(line);
  } catch (const std::exception&) {
    std::cout << "(not a number, ignored)\n";
    return fallback;
  }
}

Priority ask_priority() {
  // End of stream is treated as the default medium priority; later inputs then hit EOF and exit safely.
  const int choice =
      read_int("Priority 1=low 2=medium 3=high (Enter=medium): ", 2).value_or(2);
  switch (choice) {
    case 1:
      return Priority::Low;
    case 3:
      return Priority::High;
    default:
      return Priority::Medium;
  }
}

// Validate "YYYY-MM-DD": separators in the right positions, digits everywhere else, and
// month/day within a sensible range.
bool is_valid_due_date(const std::string& text) {
  if (text.size() != 10) {
    return false;
  }
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (i == 4 || i == 7) {
      if (text[i] != '-') return false;
    } else if (std::isdigit(static_cast<unsigned char>(text[i])) == 0) {
      return false;
    }
  }
  const int month = std::stoi(text.substr(5, 2));
  const int day = std::stoi(text.substr(8, 2));
  return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

std::string ask_due_date() {
  // Keep prompting until the input is valid, or the user presses Enter to skip.
  while (true) {
    std::cout << "Due date (format YYYY-MM-DD, press Enter to skip): ";
    std::string due_date;
    if (!std::getline(std::cin, due_date)) {
      return {};  // input stream ended (e.g. Ctrl+D)
    }
    if (due_date.empty() || is_valid_due_date(due_date)) {
      return due_date;
    }
    std::cout << "(bad format, please enter it as YYYY-MM-DD)\n";
  }
}

// Print a single task: id, priority, title, created time and due date.
void print_task(const Task& task) {
  const char* mark = (task.priority == Priority::High) ? "!!!" : "";
  std::cout << "  [" << (task.done ? "x" : " ") << "] #" << task.id << "  ["
            << todo::priority_to_text(task.priority) << "] " << mark
            << task.title << "  (" << task.created_at << ")"
            << "  due " << (task.due_date.empty() ? "none" : task.due_date)
            << "\n";
}

// Print a batch of tasks; show a message when the list is empty.
void print_tasks(const std::vector<Task>& tasks) {
  if (tasks.empty()) {
    std::cout << "(no tasks)\n";
    return;
  }
  for (const Task& task : tasks) {
    print_task(task);
  }
}

void print_menu() {
  // R"(...)" raw string: newlines and backslashes are kept literally.
  std::cout << R"(
========================================
              Todo Manager
========================================
 1. Add task
 2. View all tasks
 3. Mark task as done
 4. Delete task
 5. List pending tasks
 6. Show statistics
 7. Search tasks
 8. Undo delete
 0. Exit
========================================
)";
}

}  // namespace

// Print the menu and loop over the user's choices until exit.
int main(int argc, char** argv) {
  // argv[0] is the program name; real arguments start at argv[1].
  // Use the command-line path if given, otherwise the compile-time default.
  const std::string data_path = (argc > 1) ? argv[1] : TODO_DATA_FILE;

  // Assemble: hand the Storage to the TaskManager.
  TaskManager manager{todo::Storage{data_path}};
  manager.load();
  std::cout << "Data file: " << data_path << "\n";

  bool running = true;
  while (running) {
    print_menu();
    // nullopt means the input stream ended (e.g. Ctrl+D): exit right away, do not keep
    // printing the menu.
    const std::optional<int> choice = read_int("Choose > ", -1);
    if (!choice) {
      std::cout << "(input ended, exiting)\n";
      break;
    }

    switch (*choice) {
      case 1: {
        std::cout << "Task title: ";
        std::string title;
        std::getline(std::cin, title);
        if (title.empty()) {
          std::cout << "(title cannot be empty)\n";
          break;
        }
        const int id = manager.add(title, ask_priority(), ask_due_date());
        std::cout << "Added #" << id << "\n";
        break;
      }
      case 2:
        print_tasks(manager.all());
        break;
      case 3: {
        const int id = read_int("Which id to complete: ", -1).value_or(-1);
        std::cout << (manager.complete(id) ? "Completed ✅" : "No such id")
                  << "\n";
        break;
      }
      case 4: {
        const int id = read_int("Which id to delete: ", -1).value_or(-1);
        std::cout << (manager.remove(id) ? "Deleted 🗑 (menu 8 undoes it)"
                                         : "No such id")
                  << "\n";
        break;
      }
      case 5:
        print_tasks(manager.pending());
        break;
      case 6: {
        const std::vector<Task> all = manager.all();
        const std::size_t done = manager.count_done();
        std::cout << "Total " << all.size() << ": " << done << " done, "
                  << (all.size() - done) << " pending\n";
        break;
      }
      case 7: {
        std::cout << "Keyword to search for: ";
        std::string keyword;
        std::getline(std::cin, keyword);
        if (keyword.empty()) {
          std::cout << "(keyword cannot be empty)\n";
          break;
        }
        print_tasks(manager.search(keyword));
        break;
      }
      case 8: {
        const std::optional<Task> restored = manager.undo_remove();
        if (restored) {
          std::cout << "Restored #" << restored->id << " " << restored->title
                    << "\n";
        } else {
          std::cout << "(nothing to undo)\n";
        }
        break;
      }
      case 0:
        std::cout << "Bye!\n";
        running = false;
        break;
      default:
        std::cout << "(please enter a number from the menu)\n";
        break;
    }
  }
  return 0;
}
