#pragma once

// Small helpers shared by the tests. GoogleTest has no equivalent of pytest's
// tmp_path, so file-based tests create their own temporary directory instead.

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace todo_test {

// A unique temporary directory that is removed again when it goes out of scope
// (RAII).
class TempDir {
 public:
  TempDir() {
    static std::atomic<unsigned> counter{0};
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() /
            ("todo_test_" + std::to_string(stamp) + "_" +
             std::to_string(++counter));
    std::filesystem::create_directories(path_);
  }

  ~TempDir() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  const std::filesystem::path& path() const { return path_; }

  // A file inside the temporary directory.
  std::filesystem::path file(const std::string& name) const {
    return path_ / name;
  }

 private:
  std::filesystem::path path_;
};

// Read a whole file as text; returns "" when it cannot be opened.
inline std::string read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

// Write text to a file, replacing any previous content.
inline void write_file(const std::filesystem::path& path,
                       const std::string& text) {
  std::ofstream output(path, std::ios::trunc);
  output << text;
}

}  // namespace todo_test
