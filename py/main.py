"""Entry point for the todo manager.

This file does one thing only: talk to the user.
It does not care how tasks are stored, it just hands the user's input to TaskManager.

How to run:
    cd py
    python3 main.py

You can also point it at another data file:
    python3 main.py /tmp/my_tasks.json
"""

from __future__ import annotations

import sys
from datetime import datetime
from pathlib import Path

# These are the project's own modules. Because todo is a package, we use the from todo.xxx form.
from todo import TIME_FORMAT, JsonStorage, Priority, TaskManager

# __file__ is "the path of this .py file itself".
# Use it to locate data/tasks.json in code, so the right file is found no matter which directory
# you run from.
DEFAULT_DATA_FILE = Path(__file__).resolve().parent / "data" / "tasks.json"

MENU = """
========================================
              Todo Manager
========================================
 1. Add task
 2. View all tasks
 3. Mark task as done
 4. Delete task
 5. List pending tasks
 6. Show statistics
 7. Search tasks
 0. Exit
========================================"""


def read_int(prompt: str, fallback: int) -> int:
    """Read one line of input and convert it to an integer.

    The user may type letters where a number is expected; the program should not crash.
    fallback is "what to use when the input cannot be read".
    """
    raw = input(prompt).strip()
    try:
        return int(raw)
    except ValueError:
        print("(not a number, ignored)")
        return fallback


def ask_priority() -> Priority:
    """Ask the user for a priority. 1=low 2=medium 3=high."""
    choice = read_int("Priority 1=low 2=medium 3=high (Enter=medium): ", 2)
    mapping = {1: Priority.LOW, 2: Priority.MEDIUM, 3: Priority.HIGH}
    return mapping.get(choice, Priority.MEDIUM)


def ask_due_date() -> datetime | None:
    """Ask the user for a due date. Pressing Enter means no due date."""
    raw = input("Due date (format 2026-10-05 18:00, Enter=none): ").strip()
    if not raw:
        return None
    try:
        return datetime.strptime(raw, TIME_FORMAT)  # noqa DTZ007
    except ValueError:
        print("(bad format, this task will have no due date)")
        return None


def print_tasks(tasks: list) -> None:
    """Print a list of tasks neatly."""
    if not tasks:
        print("(no tasks)")
        return
    for task in tasks:
        print("  " + str(task))


def main() -> None:
    # The first command-line argument can override the default data file location.
    data_file = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_DATA_FILE

    # Assemble the two parts: storage layer -> business layer. This is "dependency injection".
    storage = JsonStorage(data_file)
    manager = TaskManager(storage)
    manager.load()

    print(f"Data file: {data_file}")

    while True:
        print(MENU)
        choice = read_int("Choose > ", -1)

        if choice == 1:
            title = input("Task title: ").strip()
            if not title:
                print("(title cannot be empty)")
                continue
            priority = ask_priority()
            due_date = ask_due_date()
            task = manager.add(title, priority, due_date)
            print(f"Added #{task.id}")

        elif choice == 2:
            print_tasks(manager.all())

        elif choice == 3:
            task_id = read_int("Which id to complete: ", -1)
            print("Completed ✅" if manager.complete(task_id) else "No such id")

        elif choice == 4:
            task_id = read_int("Which id to delete: ", -1)
            print("Deleted 🗑" if manager.remove(task_id) else "No such id")

        elif choice == 5:
            print_tasks(manager.pending())

        elif choice == 6:
            stats = manager.stats()
            print(
                f"Total {stats['total']}: {stats['done']} done, {stats['pending']} pending"
            )

        elif choice == 7:
            keyword = input("Enter keyword: ").strip()
            results = manager.search(keyword)
            if not results:
                print("No matching tasks")
            else:
                print_tasks(results)

        elif choice == 0:
            print("Bye!")
            break

        else:
            print("(please enter a number from the menu)")


# This is the standard Python idiom:
# main() runs only when this file is run directly, not when it is imported by someone else.
if __name__ == "__main__":
    main()
