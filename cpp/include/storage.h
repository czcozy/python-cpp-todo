#pragma once

#include <string>
#include <vector>

#include "task.h"

namespace todo {

// Storage layer: only reads and writes the disk. Switching to a database or another format
// means changing this one class.
class Storage {
 public:
  // Remember the data file path (explicit blocks implicit conversions like Storage s = "a.txt";).
  explicit Storage(std::string path);

  // Read all tasks from the file; a missing file yields an empty list (const = does not modify
  // this object's state).
  std::vector<Task> load() const;

  // Write all tasks back to the file (overwriting the previous content).
  void save(const std::vector<Task>& tasks) const;

  // Return the data file path.
  const std::string& path() const { return path_; }

 private:
  std::string path_;
};

}  // namespace todo
