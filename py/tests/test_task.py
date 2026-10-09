"""Data model layer tests: Task defaults, serialization (to_dict) and deserialization (from_dict)."""

from __future__ import annotations

import json
from datetime import datetime

import pytest
from todo import Priority, Task
from todo.task import TIME_FORMAT

# Two timestamps used repeatedly in the tests; they match raw_task()'s defaults in conftest.
CREATED_AT = datetime(2026, 10, 3, 15, 30)  # noqa DTZ001
DUE_DATE = datetime(2026, 10, 5, 18, 0)  # noqa DTZ001


class TestDefaults:
    """The defaults @dataclass provides; don't let later changes quietly alter them."""

    def test_priority_defaults_to_medium(self) -> None:
        assert Task(id=1, title="Buy milk").priority is Priority.MEDIUM

    def test_done_defaults_to_false(self) -> None:
        assert Task(id=1, title="Buy milk").done is False

    def test_due_date_defaults_to_none(self) -> None:
        # None means "no due date"; this is the field added in exercise 2, unset by default.
        assert Task(id=1, title="Buy milk").due_date is None

    def test_created_at_defaults_to_now(self) -> None:
        before = datetime.now()  # noqa DTZ005
        task = Task(id=1, title="Buy milk")
        after = datetime.now()  # noqa DTZ005
        assert before <= task.created_at <= after

    def test_every_task_gets_its_own_created_at(self) -> None:
        # The point of field(default_factory=...): the default is computed per new object.
        # Written as created_at: datetime = datetime.now(), every task would share one timestamp.
        first = Task(id=1, title="a")
        second = Task(id=2, title="b")
        assert first.created_at is not second.created_at


class TestPriority:
    def test_time_format_matches_readme(self) -> None:
        assert CREATED_AT.strftime(TIME_FORMAT) == "2026-10-03 15:30"

    @pytest.mark.parametrize(
        ("priority", "text"),
        [
            (Priority.LOW, "low"),
            (Priority.MEDIUM, "medium"),
            (Priority.HIGH, "high"),
        ],
    )
    def test_label(self, priority: Priority, text: str) -> None:
        assert priority.label == text

    def test_is_still_an_int(self) -> None:
        # The benefit of IntEnum: readable as a constant, yet usable for saving and sorting.
        assert Priority.HIGH == 2
        assert sorted([Priority.HIGH, Priority.LOW, Priority.MEDIUM]) == [
            Priority.LOW,
            Priority.MEDIUM,
            Priority.HIGH,
        ]


class TestToDict:
    def test_all_fields_are_serialized(self) -> None:
        task = Task(
            id=7,
            title="Write code",
            priority=Priority.HIGH,
            done=True,
            created_at=CREATED_AT,
            due_date=DUE_DATE,
        )
        assert task.to_dict() == {
            "id": 7,
            "title": "Write code",
            "priority": 2,
            "done": True,
            "created_at": "2026-10-03 15:30",
            "due_date": "2026-10-05 18:00",
        }

    def test_datetime_is_converted_to_text(self) -> None:
        # JSON cannot store datetime objects, so they must become strings before saving.
        data = Task(id=1, title="x", created_at=CREATED_AT, due_date=DUE_DATE).to_dict()
        assert isinstance(data["created_at"], str)
        assert isinstance(data["due_date"], str)

    def test_no_due_date_is_serialized_as_none(self) -> None:
        assert Task(id=1, title="x").to_dict()["due_date"] is None

    def test_seconds_are_dropped(self) -> None:
        # The format is "%Y-%m-%d %H:%M", so seconds are cut off (minute precision is enough).
        task = Task(id=1, title="x", created_at=datetime(2026, 10, 3, 15, 30, 59))  # noqa DTZ001
        assert task.to_dict()["created_at"] == "2026-10-03 15:30"

    def test_result_can_be_dumped_by_json(self) -> None:
        # This is the only reason to_dict exists: the result must be json.dumps-able.
        task = Task(id=1, title="Buy milk", created_at=CREATED_AT, due_date=DUE_DATE)
        dumped = json.dumps(task.to_dict(), ensure_ascii=False)
        assert json.loads(dumped)["title"] == "Buy milk"


class TestFromDict:
    def test_round_trip_keeps_every_field(self) -> None:
        original = Task(
            id=3,
            title="Write code",
            priority=Priority.HIGH,
            done=True,
            created_at=CREATED_AT,
            due_date=DUE_DATE,
        )
        assert Task.from_dict(original.to_dict()) == original

    def test_missing_fields_fall_back_to_defaults(self, raw_task) -> None:
        # Simulate a file saved by an older version: only id and title.
        data = raw_task()
        for key in ("priority", "done", "created_at", "due_date"):
            data.pop(key)

        task = Task.from_dict(data)
        assert (task.id, task.title) == (1, "Buy milk")
        assert task.priority is Priority.MEDIUM
        assert task.done is False
        assert task.due_date is None
        assert isinstance(task.created_at, datetime)

    def test_due_date_can_be_null(self, raw_task) -> None:
        assert Task.from_dict(raw_task(due_date=None)).due_date is None

    def test_due_date_text_is_parsed(self, raw_task) -> None:
        assert (
            Task.from_dict(raw_task(due_date="2026-10-05 18:00")).due_date == DUE_DATE
        )

    @pytest.mark.parametrize(
        "bad_value",
        ["", "   ", "not a time", "2026/10/05", "2026-13-45 99:99", None, 20261005],
    )
    def test_bad_due_date_means_no_deadline(self, raw_task, bad_value) -> None:
        # Users can edit the data file by hand; a bad format should not crash the program.
        assert Task.from_dict(raw_task(due_date=bad_value)).due_date is None

    def test_bad_created_at_falls_back_to_now(self, raw_task) -> None:
        before = datetime.now()  # noqa DTZ005
        task = Task.from_dict(raw_task(created_at="broken time"))
        assert before <= task.created_at <= datetime.now()  # noqa DTZ005

    def test_unknown_fields_are_ignored(self, raw_task) -> None:
        # When fields are added or renamed later, extra keys in old files must not crash things.
        task = Task.from_dict(raw_task(color="red", tags=["a", "b"]))
        assert task.id == 1

    def test_string_id_is_accepted(self, raw_task) -> None:
        assert Task.from_dict(raw_task(id="8")).id == 8


class TestStr:
    def test_unfinished_uses_empty_box(self) -> None:
        assert str(Task(id=1, title="x")).startswith("⬜")

    def test_finished_uses_check_mark(self) -> None:
        assert str(Task(id=1, title="x", done=True)).startswith("✅")

    def test_high_priority_is_flagged(self) -> None:
        assert "!!!" in str(Task(id=1, title="x", priority=Priority.HIGH))

    def test_title_and_id_are_shown(self) -> None:
        text = str(Task(id=42, title="Buy milk"))
        assert "Buy milk" in text
        assert "#42" in text

    def test_due_date_is_shown(self) -> None:
        assert "2026-10-05 18:00" in str(Task(id=1, title="x", due_date=DUE_DATE))

    def test_no_due_date_shows_placeholder(self) -> None:
        assert "no due" in str(Task(id=1, title="x"))
