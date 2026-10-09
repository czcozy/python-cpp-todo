// Tests for the Storage layer: missing, blank, corrupt and hand-edited files must
// all be survivable.

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "storage.h"
#include "task.h"
#include "test_support.h"

namespace {

using todo::Priority;
using todo::Storage;
using todo::Task;
using todo_test::read_file;
using todo_test::TempDir;
using todo_test::write_file;

TEST(StorageLoad, MissingFileReturnsEmptyList) {
  const TempDir dir;
  const Storage storage(dir.file("tasks.json").string());
  EXPECT_TRUE(storage.load().empty());
}

TEST(StorageLoad, BlankFileReturnsEmptyList) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const Storage storage(path.string());

  for (const std::string content : {"", "   ", "\n\t  \n"}) {
    write_file(path, content);
    EXPECT_TRUE(storage.load().empty()) << "content: [" << content << "]";
  }
}

TEST(StorageLoad, ReturnsAValidListUnchanged) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  write_file(path, R"([
    {"id": 1, "title": "Buy milk", "priority": 1, "done": false,
     "created_at": "2026-10-03 15:30", "due_date": null},
    {"id": 2, "title": "Sleep", "priority": 2, "done": true,
     "created_at": "2026-10-04 09:00", "due_date": "2026-10-05"}
  ])");

  const std::vector<Task> tasks = Storage(path.string()).load();
  ASSERT_EQ(tasks.size(), 2u);
  EXPECT_EQ(tasks[0].id, 1);
  EXPECT_EQ(tasks[0].title, "Buy milk");
  EXPECT_EQ(tasks[0].priority, Priority::Medium);
  EXPECT_FALSE(tasks[0].done);
  EXPECT_EQ(tasks[0].created_at, "2026-10-03 15:30");
  EXPECT_TRUE(tasks[0].due_date.empty());
  EXPECT_EQ(tasks[1].id, 2);
  EXPECT_EQ(tasks[1].priority, Priority::High);
  EXPECT_TRUE(tasks[1].done);
  EXPECT_EQ(tasks[1].due_date, "2026-10-05");
}

TEST(StorageLoad, NonListJsonReturnsEmptyListWithoutBackup) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const Storage storage(path.string());

  for (const std::string content :
       {"true", "123", "\"abc\"", "null", "{\"id\": 1}"}) {
    write_file(path, content);
    EXPECT_TRUE(storage.load().empty()) << "content: " << content;
    // Still valid JSON, just not a task list, so it must not be treated as a bad file.
    EXPECT_FALSE(std::filesystem::exists(path.string() + ".bak"));
  }
}

TEST(StorageLoad, DropsItemsThatAreNotObjects) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  write_file(path,
             R"([{"id": 1, "title": "Buy milk"}, 1, "x", null, [1, 2]])");

  const std::vector<Task> tasks = Storage(path.string()).load();
  ASSERT_EQ(tasks.size(), 1u);
  EXPECT_EQ(tasks[0].title, "Buy milk");
}

TEST(StorageLoad, DropsEntriesWithUnusableId) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  // 0, negative, missing and non-integer ids are all dropped; only a real id survives.
  write_file(path, R"([
    {"id": 0, "title": "zero"},
    {"id": -3, "title": "negative"},
    {"title": "missing"},
    {"id": "1", "title": "string id"},
    {"id": 1.5, "title": "float id"},
    {"id": 9, "title": "good"}
  ])");

  const std::vector<Task> tasks = Storage(path.string()).load();
  ASSERT_EQ(tasks.size(), 1u);
  EXPECT_EQ(tasks[0].id, 9);
  EXPECT_EQ(tasks[0].title, "good");
}

TEST(StorageLoad, FallsBackWhenFieldTypesAreWrong) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  // A hand-edited file can hold any type; every bad field falls back to its default.
  write_file(path, R"([{
    "id": 3,
    "title": 42,
    "priority": "high",
    "done": 1,
    "created_at": 20261003,
    "due_date": null
  }])");

  const std::vector<Task> tasks = Storage(path.string()).load();
  ASSERT_EQ(tasks.size(), 1u);
  EXPECT_EQ(tasks[0].id, 3);
  EXPECT_TRUE(tasks[0].title.empty());
  EXPECT_EQ(tasks[0].priority, Priority::Medium);
  EXPECT_FALSE(tasks[0].done);
  EXPECT_TRUE(tasks[0].created_at.empty());
  EXPECT_TRUE(tasks[0].due_date.empty());
}

TEST(StorageLoad, CorruptFileIsBackedUpAndTreatedAsEmpty) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const std::string bad = "{\"id\": 1,";
  write_file(path, bad);

  testing::internal::CaptureStderr();
  const std::vector<Task> tasks = Storage(path.string()).load();
  const std::string errors = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(tasks.empty());
  EXPECT_EQ(read_file(path.string() + ".bak"), bad);
  EXPECT_FALSE(std::filesystem::exists(path));  // the original was renamed away
  EXPECT_NE(errors.find("malformed"), std::string::npos);
  EXPECT_NE(errors.find("backed up to"), std::string::npos);
}

TEST(StorageLoad, SecondCorruptFileOverwritesTheOldBackup) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const Storage storage(path.string());

  testing::internal::CaptureStderr();
  write_file(path, "first corruption");
  storage.load();
  write_file(path, "second corruption");
  storage.load();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(read_file(path.string() + ".bak"), "second corruption");
}

TEST(StorageLoad, CanSaveAgainAfterACorruptFile) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  write_file(path, "bad data");

  const Storage storage(path.string());
  testing::internal::CaptureStderr();
  storage.load();
  testing::internal::GetCapturedStderr();

  Task task(1, "Buy milk", Priority::Medium, "2026-10-03 15:30", "");
  storage.save({task});

  EXPECT_EQ(Storage(path.string()).load().size(), 1u);
}

TEST(StorageSave, CreatesMissingParentDirectories) {
  const TempDir dir;
  const auto path = dir.path() / "deep" / "nested" / "tasks.json";
  Task task(1, "Buy milk", Priority::Medium, "t", "");

  Storage(path.string()).save({task});

  EXPECT_TRUE(std::filesystem::exists(path));
}

TEST(StorageSave, WritesIndentedJson) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  Task task(1, "Buy milk", Priority::Medium, "t", "");

  Storage(path.string()).save({task});

  // indent=2: the array is indented two spaces and the keys another two.
  EXPECT_NE(read_file(path).find("\n    \"title\""), std::string::npos);
}

TEST(StorageSave, EmptyListBecomesJsonArray) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");

  Storage(path.string()).save({});

  EXPECT_EQ(read_file(path), "[]\n");
}

TEST(StorageSave, OverwritesPreviousContent) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const Storage storage(path.string());

  storage.save({Task(1, "old", Priority::Medium, "t", "")});
  storage.save({Task(2, "new", Priority::Medium, "t", "")});

  const std::vector<Task> tasks = storage.load();
  ASSERT_EQ(tasks.size(), 1u);
  EXPECT_EQ(tasks[0].id, 2);
  EXPECT_EQ(tasks[0].title, "new");
}

TEST(StorageSave, WritesNullForAMissingDueDate) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  Storage(path.string()).save({Task(1, "Buy milk", Priority::Medium, "t", "")});

  const nlohmann::json data = nlohmann::json::parse(read_file(path));
  EXPECT_TRUE(data.at(0).at("due_date").is_null());
}

TEST(StorageSave, KeepsNonAsciiTextReadable) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  Storage(path.string()).save(
      {Task(1, "Buy milk 🥛", Priority::Medium, "t", "")});

  // ensure non-ASCII is written literally rather than as \uXXXX escapes.
  EXPECT_NE(read_file(path).find("Buy milk 🥛"), std::string::npos);
}

TEST(StorageSave, SaveThenLoadRoundTrip) {
  const TempDir dir;
  const auto path = dir.file("tasks.json");
  const Storage storage(path.string());
  Task first(1, "Buy milk", Priority::High, "2026-10-03 15:30", "");
  Task second(2, "Sleep", Priority::Low, "2026-10-04 09:00", "2026-10-05");
  second.done = true;

  storage.save({first, second});
  const std::vector<Task> loaded = storage.load();

  ASSERT_EQ(loaded.size(), 2u);
  EXPECT_EQ(loaded[0].title, "Buy milk");
  EXPECT_EQ(loaded[0].priority, Priority::High);
  EXPECT_EQ(loaded[1].title, "Sleep");
  EXPECT_EQ(loaded[1].priority, Priority::Low);
  EXPECT_TRUE(loaded[1].done);
  EXPECT_EQ(loaded[1].due_date, "2026-10-05");
}

TEST(StoragePath, ReturnsTheGivenPath) {
  const Storage storage("/tmp/some/tasks.json");
  EXPECT_EQ(storage.path(), "/tmp/some/tasks.json");
}

}  // namespace
