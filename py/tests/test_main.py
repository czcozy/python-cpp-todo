"""Entry point tests: the input-parsing helpers, plus an end-to-end check that runs the whole
program.
"""

from __future__ import annotations

import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import main as app
import pytest
from todo import Priority, Task

PY_DIR = Path(__file__).resolve().parent.parent


def _fake_input(monkeypatch, text: str) -> None:
    """Replace input() with "the user already typed this line"."""
    monkeypatch.setattr("builtins.input", lambda prompt="": text)


def _run_app(data_file: Path, user_input: str) -> subprocess.CompletedProcess:
    """Run main.py end to end in a subprocess, just like a real user would."""
    env = {**os.environ, "PYTHONIOENCODING": "utf-8", "PYTHONUTF8": "1"}
    return subprocess.run(
        [sys.executable, str(PY_DIR / "main.py"), str(data_file)],
        input=user_input,
        capture_output=True,
        text=True,
        encoding="utf-8",
        cwd=PY_DIR,
        env=env,
        timeout=30,
        check=False,
    )


class TestReadInt:
    def test_parses_a_number(self, monkeypatch) -> None:
        _fake_input(monkeypatch, "42")
        assert app.read_int("Choose > ", -1) == 42

    def test_trims_spaces(self, monkeypatch) -> None:
        _fake_input(monkeypatch, "  7  ")
        assert app.read_int("Choose > ", -1) == 7

    @pytest.mark.parametrize("text", ["", "abc", "3.5", "1 2"])
    def test_bad_input_returns_fallback(self, monkeypatch, capsys, text: str) -> None:
        _fake_input(monkeypatch, text)
        assert app.read_int("Choose > ", -1) == -1
        assert "not a number" in capsys.readouterr().out


class TestAskPriority:
    @pytest.mark.parametrize(
        ("text", "expected"),
        [
            ("1", Priority.LOW),
            ("2", Priority.MEDIUM),
            ("3", Priority.HIGH),
            ("", Priority.MEDIUM),  # pressing Enter = medium
            ("99", Priority.MEDIUM),  # random input = medium
            ("abc", Priority.MEDIUM),
        ],
    )
    def test_choices(self, monkeypatch, capsys, text: str, expected: Priority) -> None:
        _fake_input(monkeypatch, text)
        assert app.ask_priority() is expected
        capsys.readouterr()  # swallow messages such as "not a number"


class TestAskDueDate:
    def test_parses_valid_text(self, monkeypatch) -> None:
        _fake_input(monkeypatch, "2026-10-05 18:00")
        assert app.ask_due_date() == datetime(2026, 10, 5, 18, 0)  # noqa DTZ001

    @pytest.mark.parametrize("text", ["", "   "])
    def test_empty_means_no_deadline(self, monkeypatch, text: str) -> None:
        _fake_input(monkeypatch, text)
        assert app.ask_due_date() is None

    @pytest.mark.parametrize("text", ["tomorrow", "2026/10/05", "2026-10-05"])
    def test_bad_format_means_no_deadline(self, monkeypatch, capsys, text: str) -> None:
        _fake_input(monkeypatch, text)
        assert app.ask_due_date() is None
        assert "bad format" in capsys.readouterr().out


class TestPrintTasks:
    def test_empty_list(self, capsys) -> None:
        app.print_tasks([])
        assert "no tasks" in capsys.readouterr().out

    def test_each_task_on_its_own_line(self, capsys) -> None:
        app.print_tasks([Task(id=1, title="Buy milk"), Task(id=2, title="Sleep")])
        lines = capsys.readouterr().out.strip().splitlines()
        assert len(lines) == 2
        assert "Buy milk" in lines[0]
        assert "Sleep" in lines[1]


class TestCommandLineFlow:
    """End to end: start the real program, feed it a series of menu inputs, inspect output and file."""

    def test_added_task_survives_a_restart(self, tmp_path) -> None:
        data_file = tmp_path / "tasks.json"

        first_run = _run_app(data_file, "1\nBuy milk\n3\n2026-10-05 18:00\n2\n0\n")
        assert first_run.returncode == 0
        assert "Added #1" in first_run.stdout
        assert "Buy milk" in first_run.stdout
        assert "2026-10-05 18:00" in first_run.stdout

        # Second run: it reads from the file again, so the task should still be there.
        second_run = _run_app(data_file, "2\n0\n")
        assert second_run.returncode == 0
        assert "Buy milk" in second_run.stdout

    def test_completed_task_is_persisted(self, tmp_path) -> None:
        data_file = tmp_path / "tasks.json"

        _run_app(data_file, "1\nBuy milk\n2\n\n3\n1\n0\n")
        second_run = _run_app(data_file, "2\n0\n")

        assert "✅" in second_run.stdout

    def test_new_ids_continue_after_restart(self, tmp_path) -> None:
        data_file = tmp_path / "tasks.json"

        _run_app(data_file, "1\none\n\n\n1\ntwo\n\n\n0\n")
        result = _run_app(data_file, "1\nthree\n\n\n2\n0\n")

        assert "Added #3" in result.stdout

    def test_corrupt_data_file_is_backed_up_and_program_still_runs(
        self, tmp_path
    ) -> None:
        data_file = tmp_path / "tasks.json"
        data_file.write_text("corrupt content{{{", encoding="utf-8")

        result = _run_app(data_file, "0\n")

        assert result.returncode == 0
        assert "malformed" in result.stdout
        assert (tmp_path / "tasks.json.bak").exists()

    def test_empty_title_is_rejected(self, tmp_path) -> None:
        data_file = tmp_path / "tasks.json"

        result = _run_app(data_file, "1\n\n0\n")

        assert "title cannot be empty" in result.stdout
        assert not data_file.exists() or data_file.read_text(encoding="utf-8") == "[]"

    def test_search_menu_item_finds_a_task(self, tmp_path) -> None:
        data_file = tmp_path / "tasks.json"

        result = _run_app(data_file, "1\nBuy milk\n\n\n7\nmilk\n0\n")

        assert result.returncode == 0
        assert "Buy milk" in result.stdout  # the search result printed this task
        assert "No matching tasks" not in result.stdout
