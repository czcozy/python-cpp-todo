"""Fixtures shared by all tests.

One iron rule: tests never touch the real data/tasks.json.
Any test that reads or writes files uses pytest's tmp_path;
it creates a separate temporary directory per test function and cleans up afterwards.
"""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path

import pytest
from todo import JsonStorage, TaskManager


@pytest.fixture
def storage_path(tmp_path: Path) -> Path:
    """A path to a temporary data file that does not exist yet."""
    return tmp_path / "tasks.json"


@pytest.fixture
def storage(storage_path: Path) -> JsonStorage:
    """A storage layer pointing at the temporary file."""
    return JsonStorage(storage_path)


@pytest.fixture
def manager(storage: JsonStorage) -> TaskManager:
    """An empty TaskManager, fully isolated from real data."""
    task_manager = TaskManager(storage)
    task_manager.load()
    return task_manager


@pytest.fixture
def raw_task() -> Callable[..., dict]:
    """Build one raw dict "as read from a file"; override any field via keyword arguments.

    Usage: raw_task(), raw_task(id=2, title="Sleep"), raw_task(due_date=None)
    """

    def _make(**overrides: object) -> dict:
        data: dict = {
            "id": 1,
            "title": "Buy milk",
            "priority": 1,
            "done": False,
            "created_at": "2026-10-03 15:30",
            "due_date": None,
        }
        data.update(overrides)
        return data

    return _make
