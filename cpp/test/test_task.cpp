// Tests for the Task data model: defaults, the constructor, and the text
// (to_line / from_line) helpers.

#include <gtest/gtest.h>

#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "task.h"

namespace {

using todo::Priority;
using todo::Task;

TEST(TaskDefaults, AreSane) {
  const Task task;
  EXPECT_EQ(task.id, 0);
  EXPECT_TRUE(task.title.empty());
  EXPECT_EQ(task.priority, Priority::Medium);
  EXPECT_FALSE(task.done);
  EXPECT_TRUE(task.created_at.empty());
  EXPECT_TRUE(task.due_date.empty());
}

TEST(TaskConstructor, StoresEveryField) {
  const Task task(7, "Write code", Priority::High, "2026-10-03 15:30",
                  "2026-10-05");
  EXPECT_EQ(task.id, 7);
  EXPECT_EQ(task.title, "Write code");
  EXPECT_EQ(task.priority, Priority::High);
  EXPECT_EQ(task.created_at, "2026-10-03 15:30");
  EXPECT_EQ(task.due_date, "2026-10-05");
  EXPECT_FALSE(task.done);  // done is set separately, not by the constructor
}

TEST(TaskToLine, JoinsFieldsWithPipes) {
  Task task(7, "Write code", Priority::High, "2026-10-03 15:30", "2026-10-05");
  task.done = true;
  EXPECT_EQ(task.to_line(), "7|Write code|2|1|2026-10-03 15:30|2026-10-05");
}

TEST(TaskToLine, EncodesDoneAsBit) {
  Task task(1, "x", Priority::Low, "t", "");
  EXPECT_EQ(task.to_line(), "1|x|0|0|t|");
  task.done = true;
  EXPECT_EQ(task.to_line(), "1|x|0|1|t|");
}

TEST(TaskFromLine, ParsesEveryField) {
  const Task task =
      Task::from_line("7|Write code|2|1|2026-10-03 15:30|2026-10-05");
  EXPECT_EQ(task.id, 7);
  EXPECT_EQ(task.title, "Write code");
  EXPECT_EQ(task.priority, Priority::High);
  EXPECT_TRUE(task.done);
  EXPECT_EQ(task.created_at, "2026-10-03 15:30");
  EXPECT_EQ(task.due_date, "2026-10-05");
}

TEST(TaskFromLine, RoundTripsThroughToLine) {
  Task original(3, "Write code", Priority::Low, "2026-10-03 15:30", "");
  original.done = true;
  const Task parsed = Task::from_line(original.to_line());
  EXPECT_EQ(parsed.id, original.id);
  EXPECT_EQ(parsed.title, original.title);
  EXPECT_EQ(parsed.priority, original.priority);
  EXPECT_EQ(parsed.done, original.done);
  EXPECT_EQ(parsed.created_at, original.created_at);
  EXPECT_EQ(parsed.due_date, original.due_date);
}

TEST(TaskFromLine, NeedsAtLeastFourFields) {
  // Fewer than 4 fields means the line is unusable, so a default Task is returned.
  const Task task = Task::from_line("1|only|two");
  EXPECT_EQ(task.id, 0);
  EXPECT_TRUE(task.title.empty());
}

TEST(TaskFromLine, MissingOptionalFieldsStayEmpty) {
  // created_at and due_date were added later, so old lines may stop at 4 fields.
  const Task task = Task::from_line("5|Sleep|1|0");
  EXPECT_EQ(task.id, 5);
  EXPECT_EQ(task.title, "Sleep");
  EXPECT_EQ(task.priority, Priority::Medium);
  EXPECT_FALSE(task.done);
  EXPECT_TRUE(task.created_at.empty());
  EXPECT_TRUE(task.due_date.empty());
}

TEST(TaskFromLine, FiveFieldsSetsCreatedAtOnly) {
  const Task task = Task::from_line("5|Sleep|1|0|2026-10-03 15:30");
  EXPECT_EQ(task.created_at, "2026-10-03 15:30");
  EXPECT_TRUE(task.due_date.empty());
}

TEST(TaskFromLine, IgnoresExtraFields) {
  const Task task =
      Task::from_line("5|Sleep|1|0|2026-10-03 15:30|2026-10-05|ignored|more");
  EXPECT_EQ(task.id, 5);
  EXPECT_EQ(task.created_at, "2026-10-03 15:30");
  EXPECT_EQ(task.due_date, "2026-10-05");
}

TEST(TaskFromLine, StrictAboutNonNumericFields) {
  // These legacy text helpers are strict, unlike Storage: a non-numeric id throws.
  EXPECT_THROW(Task::from_line("not-a-number|Sleep|1|0"),
               std::invalid_argument);
}

TEST(NowString, HasTheDocumentedShape) {
  const std::string now = todo::now_string();
  ASSERT_EQ(now.size(), 16u);  // "YYYY-MM-DD HH:MM"
  EXPECT_EQ(now[4], '-');
  EXPECT_EQ(now[7], '-');
  EXPECT_EQ(now[10], ' ');
  EXPECT_EQ(now[13], ':');
  for (std::size_t i = 0; i < now.size(); ++i) {
    if (i == 4 || i == 7 || i == 10 || i == 13) {
      continue;
    }
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(now[i])))
        << "position " << i << " is not a digit";
  }
}

}  // namespace
