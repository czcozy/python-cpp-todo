"""Data model layer: defines what "one todo" looks like.

This layer only describes the data itself; it does not touch files or user input.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime
from enum import IntEnum


class Priority(IntEnum):
    """Priority.

    Using IntEnum instead of plain 1/2/3:
      * writing Priority.HIGH in code reads better than writing 3;
      * it is still an integer underneath, so sorting and saving are easy;
      * a typo (Priority.HIG) fails immediately, whereas a wrong number gives no warning.
    """

    LOW = 0
    MEDIUM = 1
    HIGH = 2

    @property
    def label(self) -> str:
        """Look up the human-readable label for a priority."""
        return {
            Priority.LOW: "low",
            Priority.MEDIUM: "medium",
            Priority.HIGH: "high",
        }[self]


TIME_FORMAT = "%Y-%m-%d %H:%M"


def _format_time(value: datetime) -> str:
    """datetime -> "2026-10-03 15:30". Used when saving, because JSON cannot store datetime."""
    return value.strftime(TIME_FORMAT)


def _parse_time(value: object) -> datetime | None:
    """Turn text like "2026-10-03 15:30" into a datetime.

    Users can edit the data file by hand, so be lenient here: empty values, missing fields and
    bad formats all mean "no such time" instead of crashing the program.
    """
    if not isinstance(value, str) or not value.strip():
        return None
    try:
        return datetime.strptime(value.strip(), TIME_FORMAT)  # noqa DTZ007
    except ValueError:
        return None


@dataclass
class Task:
    """One todo.

    @dataclass generates __init__, __eq__ and __repr__ for you,
    so you only declare the fields instead of writing a pile of boilerplate.

    field(default_factory=...) means "compute the default only when a new object is created".
    Writing created_at: datetime = datetime.now() would make all objects share the same
    timestamp — a very common pitfall.
    """

    id: int
    title: str
    due_date: datetime | None = None  # None means "no due date"
    priority: Priority = Priority.MEDIUM
    done: bool = False
    created_at: datetime = field(default_factory=datetime.now)

    def to_dict(self) -> dict:
        """Serialize: Task object -> a plain dict that can become JSON.

        json cannot store datetime objects, so convert them to "%Y-%m-%d %H:%M" text here.
        """
        return {
            "id": self.id,
            "title": self.title,
            "priority": int(self.priority),
            "done": self.done,
            "created_at": _format_time(self.created_at),
            "due_date": _format_time(self.due_date) if self.due_date else None,
        }

    @classmethod
    def from_dict(cls, data: dict) -> Task:
        """Deserialize: dict -> Task object.

        Data read back from a file is not necessarily trustworthy (the user may have edited it),
        so use .get() with defaults; missing fields still survive.
        """
        return cls(
            id=int(data.get("id", 0)),
            title=str(data.get("title", "")),
            priority=Priority(int(data.get("priority", Priority.MEDIUM))),
            done=bool(data.get("done", False)),
            created_at=_parse_time(data.get("created_at")) or datetime.now(),  # noqa DTZ005
            due_date=_parse_time(data.get("due_date")),
        )

    def __str__(self) -> str:
        """Controls how print(task) looks, aligned as nicely as possible."""
        mark = "✅" if self.done else "⬜"
        flag = {Priority.HIGH: "!!!", Priority.MEDIUM: "!", Priority.LOW: "·"}[
            self.priority
        ]
        created = _format_time(self.created_at)
        due = _format_time(self.due_date) if self.due_date else "no due"
        return (
            f'{mark} #{self.id:<3} {flag:<4} "{self.title}" '
            f"[{self.priority.label}] ({created})({due})"
        )
