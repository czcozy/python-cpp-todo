"""Business layer tests: id assignment, CRUD, sorting, statistics, search, persistence."""

from __future__ import annotations

import json
from datetime import datetime

from todo import Priority, Task, TaskManager
from todo.storage import JsonStorage


class TestAdd:
    def test_ids_are_sequential(self, manager: TaskManager) -> None:
        first = manager.add("Buy milk")
        second = manager.add("Sleep")
        assert (first.id, second.id) == (1, 2)
        assert (first.title, second.title) == ("Buy milk", "Sleep")

    def test_default_priority_is_medium(self, manager: TaskManager) -> None:
        assert manager.add("Buy milk").priority is Priority.MEDIUM

    def test_priority_is_kept(self, manager: TaskManager) -> None:
        assert manager.add("Buy milk", Priority.HIGH).priority is Priority.HIGH

    def test_due_date_is_kept(self, manager: TaskManager) -> None:
        due = datetime(2026, 10, 5, 18, 0)  # noqa DTZ001
        assert manager.add("Buy milk", due_date=due).due_date == due

    def test_new_task_is_saved_immediately(
        self, manager: TaskManager, storage: JsonStorage
    ) -> None:
        manager.add("Buy milk", Priority.HIGH, datetime(2026, 10, 5, 18, 0))  # noqa DTZ001

        saved = storage.load()
        assert len(saved) == 1
        assert saved[0]["title"] == "Buy milk"
        assert saved[0]["priority"] == 2
        assert saved[0]["done"] is False
        assert saved[0]["due_date"] == "2026-10-05 18:00"
        assert isinstance(saved[0]["created_at"], str)

    def test_new_task_shows_up_in_all(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        assert [task.title for task in manager.all()] == ["Buy milk"]


class TestLoad:
    def test_loads_tasks_from_file(
        self, storage: JsonStorage, storage_path, raw_task
    ) -> None:
        data = [raw_task(), raw_task(id=2, title="Sleep", done=True)]
        storage_path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")

        manager = TaskManager(storage)
        manager.load()

        assert [task.id for task in manager.all()] == [1, 2]
        assert [task.title for task in manager.all()] == ["Buy milk", "Sleep"]

    def test_next_id_is_max_plus_one_not_length(
        self, storage: JsonStorage, storage_path, raw_task
    ) -> None:
        # After deletions the "count" is smaller than the "max id"; using len() would repeat ids.
        data = [raw_task(id=1), raw_task(id=5, title="Sleep")]
        storage_path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")

        manager = TaskManager(storage)
        manager.load()

        assert manager.add("New task").id == 6

    def test_ids_do_not_repeat_after_delete_and_reload(
        self, storage: JsonStorage
    ) -> None:
        manager = TaskManager(storage)
        manager.load()
        manager.add("one")  # #1
        second = manager.add("two")  # #2
        manager.remove(second.id)

        assert manager.add("three").id == 3

        reloaded = TaskManager(storage)
        reloaded.load()
        assert reloaded.add("four").id == 4

    def test_corrupt_file_starts_from_scratch(
        self, storage: JsonStorage, storage_path
    ) -> None:
        storage_path.write_text("this is not JSON", encoding="utf-8")
        manager = TaskManager(storage)

        manager.load()

        assert manager.all() == []
        assert manager.add("Buy milk").id == 1


class TestQuery:
    def test_find_returns_the_task(self, manager: TaskManager) -> None:
        added = manager.add("Buy milk")
        assert manager.find(added.id) is added

    def test_find_returns_none_for_unknown_id(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        assert manager.find(999) is None

    def test_all_is_sorted_by_id(self, manager: TaskManager) -> None:
        # In normal use self.tasks is already ordered;
        # shuffle it by hand here to make sure all() really sorts.
        manager.tasks = [
            Task(id=3, title="c"),
            Task(id=1, title="a"),
            Task(id=2, title="b"),
        ]
        assert [task.id for task in manager.all()] == [1, 2, 3]

    def test_all_returns_a_copy(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        manager.all().clear()
        assert len(manager.all()) == 1

    def test_pending_puts_unfinished_first_then_high_priority(
        self, manager: TaskManager
    ) -> None:
        finished_low = manager.add("done-low", Priority.LOW)
        manager.complete(finished_low.id)
        pending_low = manager.add("pending-low", Priority.LOW)
        pending_high = manager.add("pending-high", Priority.HIGH)
        pending_medium = manager.add("pending-medium", Priority.MEDIUM)

        assert [task.id for task in manager.pending()] == [
            pending_high.id,
            pending_medium.id,
            pending_low.id,
            finished_low.id,
        ]

    def test_pending_of_empty_manager_is_empty(self, manager: TaskManager) -> None:
        assert manager.pending() == []


class TestMutate:
    def test_complete_marks_done_and_saves(
        self, manager: TaskManager, storage: JsonStorage
    ) -> None:
        added = manager.add("Buy milk")
        assert manager.complete(added.id) is True
        assert manager.find(added.id).done is True
        assert storage.load()[0]["done"] is True

    def test_complete_unknown_id_returns_false(self, manager: TaskManager) -> None:
        assert manager.complete(999) is False

    def test_remove_deletes_and_saves(
        self, manager: TaskManager, storage: JsonStorage
    ) -> None:
        added = manager.add("Buy milk")
        assert manager.remove(added.id) is True
        assert manager.find(added.id) is None
        assert storage.load() == []

    def test_remove_unknown_id_returns_false(self, manager: TaskManager) -> None:
        assert manager.remove(999) is False

    def test_completing_twice_is_harmless(self, manager: TaskManager) -> None:
        added = manager.add("Buy milk")
        manager.complete(added.id)
        assert manager.complete(added.id) is True


class TestStats:
    def test_empty_manager(self, manager: TaskManager) -> None:
        assert manager.stats() == {"total": 0, "done": 0, "pending": 0}

    def test_counts_after_add_and_complete(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        manager.add("Sleep")
        manager.complete(1)
        assert manager.stats() == {"total": 2, "done": 1, "pending": 1}


class TestSearch:
    def test_finds_by_substring(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        manager.add("Sleep")
        assert [task.title for task in manager.search("milk")] == ["Buy milk"]

    def test_is_case_insensitive(self, manager: TaskManager) -> None:
        manager.add("Write Code")
        assert [task.title for task in manager.search("write")] == ["Write Code"]
        assert [task.title for task in manager.search("CODE")] == ["Write Code"]

    def test_no_match_returns_empty_list(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        assert manager.search("no such keyword") == []

    def test_matches_part_of_the_title_only(self, manager: TaskManager) -> None:
        manager.add("Buy milk")
        assert manager.search("ilk") != []
        assert manager.search("fish") == []
