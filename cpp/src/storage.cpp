#include "storage.h"

#include <filesystem>  // C++17 path/directory utilities
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace todo {

namespace {

// Use ordered_json instead of the default json: the default json is a std::map internally, so
// fields get reordered alphabetically when saved; ordered_json preserves insertion order, which
// keeps the file layout consistent with the Python version.
using json = nlohmann::ordered_json;

// The helpers below follow one rule: a missing field or a wrong type falls back to a default.
// The data file can be edited by hand, and damaging it must not crash the program.
int int_field(const json& object, const char* key, int fallback) {
  const auto item = object.find(key);
  if (item == object.end() || !item->is_number_integer()) {
    return fallback;
  }
  return item->get<int>();
}

std::string string_field(const json& object, const char* key) {
  const auto item = object.find(key);
  if (item == object.end() || !item->is_string()) {
    return {};
  }
  return item->get<std::string>();
}

bool bool_field(const json& object, const char* key, bool fallback) {
  const auto item = object.find(key);
  if (item == object.end() || !item->is_boolean()) {
    return fallback;
  }
  return item->get<bool>();
}

// One JSON object -> one task; an invalid id yields an empty optional (dropped as bad data).
std::optional<Task> task_from_json(const json& object) {
  const int id = int_field(object, "id", 0);
  if (id <= 0) {
    return std::nullopt;
  }
  Task task(id, string_field(object, "title"),
            priority_from_number(int_field(object, "priority", 1)),
            string_field(object, "created_at"),
            string_field(object, "due_date"));
  task.done = bool_field(object, "done", false);
  return task;
}

// One task -> one JSON object. Field names match the Python version; the only difference is
// due_date, which is day precision here ("2026-10-08") and minute precision in Python.
json task_to_json(const Task& task) {
  json object;
  object["id"] = task.id;
  object["title"] = task.title;
  object["priority"] = priority_to_number(task.priority);
  object["done"] = task.done;
  object["created_at"] = task.created_at;
  // No due date is written as null, not as an empty string.
  object["due_date"] =
      task.due_date.empty() ? json(nullptr) : json(task.due_date);
  return object;
}

}  // namespace

Storage::Storage(std::string path) : path_(std::move(path)) {}

std::vector<Task> Storage::load() const {
  std::vector<Task> tasks;

  // ifstream: reads a file; it tries to open on construction.
  std::ifstream input(path_);
  if (!input.is_open()) {
    // A missing file is normal (first run), so return an empty list.
    return tasks;
  }

  // JSON is not a line-based format, so it cannot be read line by line; read the whole file at once.
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string text = buffer.str();
  if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
    return tasks;  // an empty file (or one with only whitespace) also counts as "no data yet"
  }

  // Third argument false: a parse failure does not throw, it returns a discarded value instead.
  const json data = json::parse(text, nullptr, false);
  if (data.is_discarded()) {
    // Bad data must not silently disappear: rename it as a backup first, then continue empty.
    std::filesystem::path backup(path_);
    backup += ".bak";
    std::error_code error;
    std::filesystem::rename(path_, backup, error);
    std::cerr << "Data file is malformed, treating it as empty for now: " << path_ << "\n";
    if (!error) {
      std::cerr << "backed up to: " << backup.string() << "\n";
    }
    return tasks;
  }

  // The top level must be an array; anything else counts as no data.
  if (!data.is_array()) {
    return tasks;
  }

  for (const json& item : data) {
    if (!item.is_object()) {
      continue;  // skip numbers, strings and the like mixed into the array
    }
    const std::optional<Task> task = task_from_json(item);
    if (task) {
      tasks.push_back(*task);
    }
  }
  return tasks;
}

void Storage::save(const std::vector<Task>& tasks) const {
  json data = json::array();
  for (const Task& task : tasks) {
    data.push_back(task_to_json(task));
  }

  // Take the parent directory and create it if missing (passing error_code means no throw on failure).
  const std::filesystem::path file_path(path_);
  if (file_path.has_parent_path()) {
    std::error_code error;
    std::filesystem::create_directories(file_path.parent_path(), error);
  }

  // ofstream: writes a file; trunc overwrites the previous content.
  std::ofstream output(path_, std::ios::trunc);
  if (!output.is_open()) {
    std::cerr << "Error: cannot write to file " << path_ << "\n";
    return;
  }

  // dump(2): two-space indent, matching Python's json.dumps(indent=2);
  // non-ASCII is not escaped by default, so text is written as-is and stays readable.
  // The last argument replaces invalid UTF-8 with U+FFFD instead of throwing and
  // interrupting this save.
  output << data.dump(2, ' ', false, json::error_handler_t::replace) << '\n';
  // No manual close(): leaving the scope flushes and closes automatically (RAII).
}

}  // namespace todo
