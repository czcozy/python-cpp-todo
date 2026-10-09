"""Storage layer tests: a missing file, empty content, corrupt JSON and hand-edited files must
all be survivable.
"""

from __future__ import annotations

import json

import pytest
from todo import JsonStorage


class TestLoad:
    def test_missing_file_returns_empty_list(self, storage: JsonStorage) -> None:
        assert storage.load() == []

    @pytest.mark.parametrize("content", ["", "   ", "\n\t  \n"])
    def test_blank_file_returns_empty_list(
        self, storage: JsonStorage, storage_path, content: str
    ) -> None:
        storage_path.write_text(content, encoding="utf-8")
        assert storage.load() == []

    def test_valid_list_is_returned_unchanged(
        self, storage: JsonStorage, storage_path, raw_task
    ) -> None:
        data = [raw_task(), raw_task(id=2, title="Sleep", done=True)]
        storage_path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
        assert storage.load() == data

    @pytest.mark.parametrize("content", ["true", "123", '"abc"', "null", '{"id": 1}'])
    def test_non_list_json_returns_empty_list(
        self, storage: JsonStorage, storage_path, content: str
    ) -> None:
        # If the top level is not an array it is not a task list; but it is still valid JSON,
        # so it must not be backed up as a "bad file".
        storage_path.write_text(content, encoding="utf-8")
        assert storage.load() == []
        assert not storage_path.with_suffix(".json.bak").exists()

    def test_non_dict_items_are_dropped(
        self, storage: JsonStorage, storage_path, raw_task
    ) -> None:
        good = raw_task()
        content = json.dumps([good, 1, "x", None, [1, 2]], ensure_ascii=False)
        storage_path.write_text(content, encoding="utf-8")
        assert storage.load() == [good]

    def test_corrupt_file_is_backed_up_and_treated_as_empty(
        self, storage: JsonStorage, storage_path, capsys
    ) -> None:
        # This is exercise 5's behavior: bad data must not silently disappear.
        storage_path.write_text('{"id": 1,', encoding="utf-8")

        assert storage.load() == []

        backup = storage_path.with_suffix(".json.bak")
        assert backup.read_text(encoding="utf-8") == '{"id": 1,'
        assert not storage_path.exists()  # after the rename the original file is gone
        assert "malformed" in capsys.readouterr().out

    def test_second_corrupt_file_overwrites_the_old_backup(
        self, storage: JsonStorage, storage_path, capsys
    ) -> None:
        backup = storage_path.with_suffix(".json.bak")

        storage_path.write_text("first corruption", encoding="utf-8")
        storage.load()
        storage_path.write_text("second corruption", encoding="utf-8")
        storage.load()

        assert backup.read_text(encoding="utf-8") == "second corruption"
        capsys.readouterr()  # swallow both messages so they do not affect later assertions

    def test_program_can_save_again_after_a_corrupt_file(
        self, storage: JsonStorage, storage_path
    ) -> None:
        storage_path.write_text("bad data", encoding="utf-8")
        storage.load()

        storage.save([{"id": 1, "title": "Buy milk"}])

        assert json.loads(storage_path.read_text(encoding="utf-8")) == [
            {"id": 1, "title": "Buy milk"}
        ]

    def test_accepts_a_string_path(self, tmp_path) -> None:
        path = tmp_path / "tasks.json"
        path.write_text("[]", encoding="utf-8")
        assert JsonStorage(str(path)).load() == []


class TestSave:
    def test_creates_missing_parent_directories(self, tmp_path) -> None:
        path = tmp_path / "deep" / "nested" / "tasks.json"
        JsonStorage(path).save([{"id": 1}])
        assert path.exists()

    def test_non_ascii_is_written_readable(
        self, storage: JsonStorage, storage_path
    ) -> None:
        # The effect of ensure_ascii=False: the file holds the character itself, not \uXXXX.
        storage.save([{"title": "Buy milk 🥛"}])
        assert "Buy milk 🥛" in storage_path.read_text(encoding="utf-8")

    def test_file_is_indented(self, storage: JsonStorage, storage_path) -> None:
        storage.save([{"title": "Buy milk"}])
        text = storage_path.read_text(encoding="utf-8")
        # indent=2: the array is indented 2 spaces and the object keys another 2, so 4 spaces.
        assert '\n    "title"' in text

    def test_empty_list_is_written_as_json_array(
        self, storage: JsonStorage, storage_path
    ) -> None:
        storage.save([])
        assert json.loads(storage_path.read_text(encoding="utf-8")) == []

    def test_save_overwrites_previous_content(
        self, storage: JsonStorage, storage_path
    ) -> None:
        storage.save([{"id": 1, "title": "old"}])
        storage.save([{"id": 2, "title": "new"}])
        assert storage.load() == [{"id": 2, "title": "new"}]

    def test_save_then_load_round_trip(self, storage: JsonStorage, raw_task) -> None:
        data = [raw_task(), raw_task(id=2, title="Sleep", done=True)]
        storage.save(data)
        assert storage.load() == data
