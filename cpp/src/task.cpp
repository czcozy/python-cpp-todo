#include "task.h"

#include <ctime>
#include <sstream>
#include <string>
#include <vector>

namespace todo {

// Anonymous namespace: everything inside is visible only in this file.
namespace {

// "a|b|c" -> {"a","b","c"}; getline consumes the separator, so the last field needs no special case.
std::vector<std::string> split(const std::string& text, char separator) {
  std::vector<std::string> parts;
  std::istringstream stream(text);
  std::string piece;
  while (std::getline(stream, piece, separator)) {
    parts.push_back(piece);
  }
  return parts;
}

}  // namespace

std::string priority_to_text(Priority p) {
  // Covers every enumerator, so changing the enum makes the compiler flag a missing branch.
  switch (p) {
    case Priority::High:
      return "high";
    case Priority::Medium:
      return "medium";
    case Priority::Low:
      return "low";
  }
  return "medium";  // fallback, to avoid a compiler warning
}

int priority_to_number(Priority p) {
  return static_cast<int>(p);  // enum class does not convert to int implicitly, so static_cast
}

Priority priority_from_number(int value) {
  switch (value) {
    case 0:
      return Priority::Low;
    case 2:
      return Priority::High;
    default:
      return Priority::Medium;  // fallback for a damaged file
  }
}

std::string Task::to_line() const {
  // Build the string with ostringstream, avoiding a long chain of +.
  std::ostringstream out;
  out << id << '|' << title << '|' << priority_to_number(priority) << '|'
      << (done ? 1 : 0) << '|' << created_at << '|' << due_date;
  return out.str();
}

Task Task::from_line(const std::string& line) {
  const std::vector<std::string> parts = split(line, '|');

  // The first 4 fields are required: the data may have been edited by hand, and a missing field
  // means this line is dropped rather than crashing.
  if (parts.size() < 4) {
    return Task{};
  }

  Task task;
  task.id = std::stoi(parts[0]);
  task.title = parts[1];
  task.priority = priority_from_number(std::stoi(parts[2]));
  task.done = (parts[3] == "1");
  // created_at / due_date were added later and may be absent in old data, so leave them empty;
  // the length must be checked before indexing, otherwise the vector is read out of bounds.
  if (parts.size() > 4) {
    task.created_at = parts[4];
  }
  if (parts.size() > 5) {
    task.due_date = parts[5];
  }
  return task;
}

std::string now_string() {
  std::time_t now = std::time(nullptr);
  std::tm local = *std::localtime(&now);  // convert to local time (not UTC)
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local);
  return std::string(buffer);
}

}  // namespace todo
