"""Business logic layer: where the real work happens.

TaskManager holds all tasks and is responsible for:
  * translating storage-layer dicts into Task objects (load);
  * assigning ids and CRUD (add / remove / complete / all / pending);
  * saving to disk right after every change (save).
"""

from __future__ import annotations

from datetime import datetime

from .storage import JsonStorage
from .task import Priority, Task


class TaskManager:
    """The task manager."""

    def __init__(self, storage: JsonStorage) -> None:
        self.storage = storage
        self.tasks: list[Task] = []
        self._next_id = 1  # a leading underscore means "internal"; outside code should not set it

    # ---------- read ----------

    def load(self) -> None:
        """Read data from the storage layer and work out the next free id."""
        self.tasks = [Task.from_dict(item) for item in self.storage.load()]
        # Use "current max id + 1" rather than the task count: after deleting tasks the count
        # shrinks, and using the length would produce duplicate ids.
        self._next_id = max((task.id for task in self.tasks), default=0) + 1

    def all(self) -> list[Task]:
        """Return all tasks (sorted by id)."""
        # A new list is returned, so external changes do not affect self.tasks — safer.
        return sorted(self.tasks, key=lambda task: task.id)

    def pending(self) -> list[Task]:
        """Sort all tasks: pending first, higher priority first within a group.

        Note: this only sorts, it does not filter; done tasks end up at the end of the list.
        """
        return sorted(self.tasks, key=lambda t: (t.done, -t.priority))

    def find(self, task_id: int) -> Task | None:
        """Find a task by id, returning None if not found.

        next(...) is "take the first element matching the condition", or None if none match.
        """
        return next((task for task in self.tasks if task.id == task_id), None)

    def stats(self) -> dict:
        """Statistics, returned as a dict for main.py to display."""
        done = sum(1 for task in self.tasks if task.done)
        return {
            "total": len(self.tasks),
            "done": done,
            "pending": len(self.tasks) - done,
        }

    # ---------- write ----------

    def add(
        self,
        title: str,
        priority: Priority = Priority.MEDIUM,
        due_date: datetime | None = None,
    ) -> Task:
        """Add a task and return the created Task.

        Not passing due_date means "no due date"; do not use a fake value like " ".
        """
        task = Task(id=self._next_id, title=title, priority=priority, due_date=due_date)
        self._next_id += 1
        self.tasks.append(task)
        self.save()
        return task

    def complete(self, task_id: int) -> bool:
        """Mark as done. Returns True on success, False if there is no such id."""
        task = self.find(task_id)
        if task is None:
            return False
        task.done = True
        self.save()
        return True

    def remove(self, task_id: int) -> bool:
        """Delete a task. Returns whether it was actually deleted."""
        task = self.find(task_id)
        if task is None:
            return False
        self.tasks.remove(task)
        self.save()
        return True

    # ---------- helpers ----------

    def save(self) -> None:
        """Hand the in-memory tasks to the storage layer to write to disk."""
        self.storage.save([task.to_dict() for task in self.tasks])

    def search(self, keyword: str) -> list[Task]:
        """Search titles by keyword, returning all hits; an empty list if none match."""
        return [task for task in self.tasks if keyword.lower() in task.title.lower()]
