// Tests for the business layer: id assignment, CRUD, sorting, counting, search,
// persistence and undo.

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "manager.h"
#include "storage.h"
#include "task.h"
#include "test_support.h"

namespace {

using todo::Priority;
using todo::Storage;
using todo::Task;
using todo::TaskManager;
using todo_test::read_file;
using todo_test::TempDir;
using todo_test::write_file;

// A manager backed by an empty temporary file, like the Python suite's `manager` fixture.
class TaskManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    path_ = dir_.file("tasks.json");
    manager_ = std::make_unique<TaskManager>(Storage(path_.string()));
    manager_->load();
  }

  // Start over with the same file, as if the program had been restarted.
  void Reload() {
    manager_ = std::make_unique<TaskManager>(Storage(path_.string()));
    manager_->load();
  }

  void WriteDataFile(const std::string& text) { write_file(path_, text); }

  std::vector<int> IdsOf(const std::vector<Task>& tasks) {
    std::vector<int> ids;
    for (const Task& task : tasks) {
      ids.push_back(task.id);
    }
    return ids;
  }

  TempDir dir_;
  std::filesystem::path path_;
  std::unique_ptr<TaskManager> manager_;
};

TEST_F(TaskManagerTest, AddAssignsSequentialIds) {
  EXPECT_EQ(manager_->add("Buy milk"), 1);
  EXPECT_EQ(manager_->add("Sleep"), 2);
  EXPECT_EQ(manager_->add("Cook"), 3);
}

TEST_F(TaskManagerTest, AddDefaultsToMediumPriority) {
  const int id = manager_->add("Buy milk");
  ASSERT_TRUE(manager_->find(id).has_value());
  EXPECT_EQ(manager_->find(id)->priority, Priority::Medium);
  EXPECT_FALSE(manager_->find(id)->done);
  EXPECT_TRUE(manager_->find(id)->due_date.empty());
}

TEST_F(TaskManagerTest, AddKeepsPriorityAndDueDate) {
  const int id = manager_->add("Buy milk", Priority::High, "2026-10-05");
  ASSERT_TRUE(manager_->find(id).has_value());
  EXPECT_EQ(manager_->find(id)->priority, Priority::High);
  EXPECT_EQ(manager_->find(id)->due_date, "2026-10-05");
}

TEST_F(TaskManagerTest, AddFillsCreatedAt) {
  const int id = manager_->add("Buy milk");
  EXPECT_FALSE(manager_->find(id)->created_at.empty());
}

TEST_F(TaskManagerTest, AddSavesImmediately) {
  manager_->add("Buy milk", Priority::High, "2026-10-05");

  const std::vector<Task> saved = Storage(path_.string()).load();
  ASSERT_EQ(saved.size(), 1u);
  EXPECT_EQ(saved[0].id, 1);
  EXPECT_EQ(saved[0].title, "Buy milk");
  EXPECT_EQ(saved[0].priority, Priority::High);
  EXPECT_FALSE(saved[0].done);
  EXPECT_EQ(saved[0].due_date, "2026-10-05");
}

TEST_F(TaskManagerTest, AddShowsUpInAll) {
  manager_->add("Buy milk");
  const std::vector<Task> all = manager_->all();
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].title, "Buy milk");
}

TEST_F(TaskManagerTest, LoadReadsTasksFromFile) {
  WriteDataFile(R"([
    {"id": 1, "title": "Buy milk", "priority": 1, "done": false,
     "created_at": "2026-10-03 15:30", "due_date": null},
    {"id": 2, "title": "Sleep", "priority": 2, "done": true,
     "created_at": "2026-10-04 09:00", "due_date": null}
  ])");
  Reload();

  const std::vector<Task> all = manager_->all();
  ASSERT_EQ(all.size(), 2u);
  EXPECT_EQ(all[0].title, "Buy milk");
  EXPECT_EQ(all[1].title, "Sleep");
  EXPECT_TRUE(all[1].done);
}

TEST_F(TaskManagerTest, NextIdIsMaxPlusOneNotTheCount) {
  // After deletions the count is smaller than the max id; using the count would repeat ids.
  WriteDataFile(R"([
    {"id": 1, "title": "Buy milk", "priority": 1, "done": false},
    {"id": 5, "title": "Sleep", "priority": 1, "done": false}
  ])");
  Reload();

  EXPECT_EQ(manager_->add("New task"), 6);
}

TEST_F(TaskManagerTest, IdsDoNotRepeatAfterDeleteAndReload) {
  manager_->add("one");                    // #1
  const int second = manager_->add("two");  // #2
  ASSERT_TRUE(manager_->remove(second));

  EXPECT_EQ(manager_->add("three"), 3);

  Reload();
  EXPECT_EQ(manager_->add("four"), 4);
}

TEST_F(TaskManagerTest, CorruptFileStartsFromScratch) {
  WriteDataFile("this is not JSON");

  testing::internal::CaptureStderr();
  Reload();
  testing::internal::GetCapturedStderr();

  EXPECT_TRUE(manager_->all().empty());
  EXPECT_EQ(manager_->add("Buy milk"), 1);
}

TEST_F(TaskManagerTest, FindReturnsTheTask) {
  const int id = manager_->add("Buy milk");
  const std::optional<Task> found = manager_->find(id);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->title, "Buy milk");
}

TEST_F(TaskManagerTest, FindReturnsEmptyForUnknownId) {
  manager_->add("Buy milk");
  EXPECT_FALSE(manager_->find(999).has_value());
}

TEST_F(TaskManagerTest, AllIsSortedById) {
  // The file is deliberately out of order to prove all() really sorts.
  WriteDataFile(R"([
    {"id": 5, "title": "e", "priority": 1, "done": false},
    {"id": 2, "title": "b", "priority": 1, "done": false},
    {"id": 9, "title": "i", "priority": 1, "done": false}
  ])");
  Reload();

  EXPECT_EQ(IdsOf(manager_->all()), (std::vector<int>{2, 5, 9}));
}

TEST_F(TaskManagerTest, AllReturnsACopy) {
  manager_->add("Buy milk");
  std::vector<Task> all = manager_->all();
  all.clear();
  EXPECT_EQ(manager_->all().size(), 1u);
}

TEST_F(TaskManagerTest, PendingPutsUnfinishedFirstThenHighPriority) {
  const int finished = manager_->add("done-low", Priority::Low);
  ASSERT_TRUE(manager_->complete(finished));
  const int low = manager_->add("pending-low", Priority::Low);
  const int high = manager_->add("pending-high", Priority::High);
  const int medium = manager_->add("pending-medium", Priority::Medium);

  // Completed tasks are excluded, and the rest are ordered by descending priority.
  EXPECT_EQ(IdsOf(manager_->pending()), (std::vector<int>{high, medium, low}));
}

TEST_F(TaskManagerTest, PendingOfEmptyManagerIsEmpty) {
  EXPECT_TRUE(manager_->pending().empty());
}

TEST_F(TaskManagerTest, CountDoneCountsCompletedTasks) {
  EXPECT_EQ(manager_->count_done(), 0u);
  const int first = manager_->add("a");
  manager_->add("b");
  manager_->add("c");
  EXPECT_EQ(manager_->count_done(), 0u);

  ASSERT_TRUE(manager_->complete(first));
  EXPECT_EQ(manager_->count_done(), 1u);
}

TEST_F(TaskManagerTest, CompleteMarksDoneAndSaves) {
  const int id = manager_->add("Buy milk");
  EXPECT_TRUE(manager_->complete(id));
  ASSERT_TRUE(manager_->find(id).has_value());
  EXPECT_TRUE(manager_->find(id)->done);
  EXPECT_TRUE(Storage(path_.string()).load()[0].done);
}

TEST_F(TaskManagerTest, CompleteUnknownIdReturnsFalse) {
  EXPECT_FALSE(manager_->complete(999));
}

TEST_F(TaskManagerTest, CompletingTwiceIsHarmless) {
  const int id = manager_->add("Buy milk");
  EXPECT_TRUE(manager_->complete(id));
  EXPECT_TRUE(manager_->complete(id));
}

TEST_F(TaskManagerTest, RemoveDeletesAndSaves) {
  const int id = manager_->add("Buy milk");
  EXPECT_TRUE(manager_->remove(id));
  EXPECT_FALSE(manager_->find(id).has_value());
  EXPECT_TRUE(Storage(path_.string()).load().empty());
  EXPECT_EQ(read_file(path_), "[]\n");
}

TEST_F(TaskManagerTest, RemoveUnknownIdReturnsFalse) {
  EXPECT_FALSE(manager_->remove(999));
}

TEST_F(TaskManagerTest, RemoveOnlyDeletesTheMatchingTask) {
  const int keep = manager_->add("keep");
  const int drop = manager_->add("drop");
  ASSERT_TRUE(manager_->remove(drop));

  EXPECT_EQ(IdsOf(manager_->all()), (std::vector<int>{keep}));
}

TEST_F(TaskManagerTest, SearchFindsBySubstring) {
  manager_->add("Buy milk");
  manager_->add("Sleep");
  const std::vector<Task> hits = manager_->search("milk");
  ASSERT_EQ(hits.size(), 1u);
  EXPECT_EQ(hits[0].title, "Buy milk");
}

TEST_F(TaskManagerTest, SearchIsCaseSensitive) {
  // Unlike the Python version, the C++ search compares raw substrings.
  manager_->add("Write Code");
  EXPECT_EQ(manager_->search("Write").size(), 1u);
  EXPECT_TRUE(manager_->search("write").empty());
}

TEST_F(TaskManagerTest, SearchNoMatchReturnsEmptyList) {
  manager_->add("Buy milk");
  EXPECT_TRUE(manager_->search("no such keyword").empty());
}

TEST_F(TaskManagerTest, SearchMatchesPartOfTheTitleOnly) {
  manager_->add("Buy milk");
  EXPECT_FALSE(manager_->search("ilk").empty());
  EXPECT_TRUE(manager_->search("fish").empty());
}

TEST_F(TaskManagerTest, UndoRemoveRestoresTheTask) {
  const int id = manager_->add("Buy milk", Priority::High, "2026-10-05");
  ASSERT_TRUE(manager_->remove(id));

  const std::optional<Task> restored = manager_->undo_remove();
  ASSERT_TRUE(restored.has_value());
  EXPECT_EQ(restored->id, id);  // ids are never reused, so the old one is kept
  EXPECT_EQ(restored->title, "Buy milk");
  EXPECT_EQ(restored->priority, Priority::High);
  EXPECT_EQ(restored->due_date, "2026-10-05");
  EXPECT_EQ(IdsOf(manager_->all()), (std::vector<int>{id}));
}

TEST_F(TaskManagerTest, UndoRemoveCanOnlyRunOnce) {
  const int id = manager_->add("Buy milk");
  ASSERT_TRUE(manager_->remove(id));
  ASSERT_TRUE(manager_->undo_remove().has_value());

  // The second undo must not restore a duplicate.
  EXPECT_FALSE(manager_->undo_remove().has_value());
  EXPECT_EQ(manager_->all().size(), 1u);
}

TEST_F(TaskManagerTest, UndoRemoveWithoutADeletionReturnsEmpty) {
  EXPECT_FALSE(manager_->undo_remove().has_value());
}

TEST_F(TaskManagerTest, UndoRemoveIsPersisted) {
  const int id = manager_->add("Buy milk");
  ASSERT_TRUE(manager_->remove(id));
  ASSERT_TRUE(manager_->undo_remove().has_value());

  EXPECT_EQ(Storage(path_.string()).load().size(), 1u);
}

}  // namespace
