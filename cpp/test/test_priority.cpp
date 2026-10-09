// Tests for the Priority enum and its three conversion helpers.

#include <gtest/gtest.h>

#include "task.h"

namespace {

using todo::Priority;
using todo::priority_from_number;
using todo::priority_to_number;
using todo::priority_to_text;

TEST(Priority, TextForEachValue) {
  EXPECT_EQ(priority_to_text(Priority::Low), "low");
  EXPECT_EQ(priority_to_text(Priority::Medium), "medium");
  EXPECT_EQ(priority_to_text(Priority::High), "high");
}

TEST(Priority, NumberForEachValue) {
  // The numbers are the on-disk representation, so they must not drift.
  EXPECT_EQ(priority_to_number(Priority::Low), 0);
  EXPECT_EQ(priority_to_number(Priority::Medium), 1);
  EXPECT_EQ(priority_to_number(Priority::High), 2);
}

TEST(Priority, FromNumberMapsKnownValues) {
  EXPECT_EQ(priority_from_number(0), Priority::Low);
  EXPECT_EQ(priority_from_number(1), Priority::Medium);
  EXPECT_EQ(priority_from_number(2), Priority::High);
}

TEST(Priority, FromNumberFallsBackToMedium) {
  // A hand-edited file can hold any number; anything unknown becomes Medium.
  EXPECT_EQ(priority_from_number(-1), Priority::Medium);
  EXPECT_EQ(priority_from_number(3), Priority::Medium);
  EXPECT_EQ(priority_from_number(99), Priority::Medium);
}

TEST(Priority, NumberAndTextRoundTripThroughNumber) {
  for (const Priority value :
       {Priority::Low, Priority::Medium, Priority::High}) {
    EXPECT_EQ(priority_from_number(priority_to_number(value)), value);
  }
}

TEST(Priority, IsOrderedByValue) {
  EXPECT_LT(static_cast<int>(Priority::Low),
            static_cast<int>(Priority::Medium));
  EXPECT_LT(static_cast<int>(Priority::Medium),
            static_cast<int>(Priority::High));
}

}  // namespace
