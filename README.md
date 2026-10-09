<div align="center">

# Todo Manager

**One CLI todo app, written twice — in pure Python 🐍 and modern C++17 ⚙️**

<sub>Same features · same layers · same JSON — only the idioms change</sub>

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](cpp/)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10%2B-3776AB?logo=python&logoColor=white)](py/)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)](cpp/CMakeLists.txt)
[![pytest — 109 cases](https://img.shields.io/badge/pytest-109%20cases-brightgreen)](py/tests/)
[![GoogleTest — 67 cases](https://img.shields.io/badge/GoogleTest-67%20cases-brightgreen)](cpp/test/)
[![PRs welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)](#contributing)
[![MIT license](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

[**English**](README.md) · [简体中文](README.zh-CN.md)

</div>

A terminal **todo list manager** implemented **twice** — once in **pure Python** and once in
**modern C++17** — with the same features, the same layered architecture and the same JSON data
format. That makes it a small, readable **Python vs C++ comparison** you can study side by side:
`@dataclass` ↔ `struct`, `None` ↔ `std::optional`, `IntEnum` ↔ `enum class`, pytest ↔ GoogleTest.

Read the short Python version in one sitting, then follow the C++ version and watch the same ideas
line up in a different language.

---

## Table of contents

- [Demo](#demo)
- [Features](#features)
- [Quick start](#quick-start)
- [Python vs C++: the same code, two idioms](#python-vs-c-the-same-code-two-idioms)
- [Who this is for](#who-this-is-for)
- [What you can learn from it](#what-you-can-learn-from-it)
- [Where to start](#where-to-start)
- [Project layout](#project-layout)
- [Layered architecture](#layered-architecture)
- [Running the tests](#running-the-tests)
- [FAQ](#faq)
- [Contributing](#contributing)
- [License](#license)

---

## Demo

A trimmed session — the menu redraws after every action:

```text
$ python3 main.py
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
========================================
Choose > 1
Task title: Buy milk
Priority 1=low 2=medium 3=high (Enter=medium): 2
Due date (format 2026-10-05 18:00, Enter=none): 2026-10-10 09:00
Added #1
Choose > 1
Task title: Write the README
Priority 1=low 2=medium 3=high (Enter=medium): 3
Due date (format 2026-10-05 18:00, Enter=none):
Added #2
Choose > 6
Total 2: 0 done, 2 pending
Choose > 2
  ⬜ #1   !    "Buy milk" [medium] (2026-10-09 12:20)(2026-10-10 09:00)
  ⬜ #2   !!!  "Write the README" [high] (2026-10-09 12:20)(no due)
Choose > 0
Bye!
```

The C++ build shows the same menu plus **8. Undo delete**.

---

## Features

Both versions share the same feature set:

- ➕ Add, 📋 list, ✅ complete, 🗑️ delete and 🔍 search tasks from a numbered terminal menu
- 🎚️ **Priorities** (low / medium / high) and 📅 **optional due dates**
- 📊 **Statistics** — total, done and pending counts (`6. Show statistics`)
- 💾 **JSON persistence**, so tasks survive a restart
- 🛡️ **Corruption safety** — a hand-edited or malformed data file is backed up instead of lost, and
  unusable entries are skipped
- 🧱 **Layered architecture** — the data model, storage, business logic and CLI are kept apart
- 🔌 **Dependency injection** — `main` wires storage into the manager; neither layer owns the other
- 🐍 **Zero setup in Python** — standard library only, no third-party runtime dependencies
- 🧪 Test suites on both sides: **109 pytest cases** and **67 GoogleTest cases**

Only in the C++ version:

- ↩️ **Undo the last delete** (`8. Undo delete`)

---

## Quick start

**Python** — needs Python 3.10+, no third-party packages:

```bash
cd py
python3 main.py
```

**C++** — needs a C++17 compiler, CMake and Ninja:

```bash
cd cpp
cmake --preset default
cmake --build --preset default
./build/todo
```

> [!TIP]
> Both versions accept a data-file path as the first argument:
> `python3 main.py /tmp/my_tasks.json` or `./build/todo /tmp/my_tasks.json`.

Both print the numbered menu shown above: type a number to act, `0` to exit. Data is written to
`data/tasks.json` inside each folder. The C++ presets need CMake 3.21+; with an older CMake use
`cmake -S . -B build` and `cmake --build build` instead.

See [`py/README.md`](py/README.md) and [`cpp/README.md`](cpp/README.md) for the details, including
how to run the tests.

---

## Python vs C++: the same code, two idioms

| Concept | Python | C++17 |
| --- | --- | --- |
| One record | `@dataclass Task` | `struct Task` |
| Priority constants | `IntEnum Priority` | `enum class Priority` |
| A collection | `list[Task]` | `std::vector<Task>` |
| Maybe no value | `None` | `std::optional<T>` |
| Module / namespace | package `todo` | `namespace todo` |
| Storage | `JsonStorage` (JSON) | `Storage` (JSON) |
| Business logic | `TaskManager` | `TaskManager` |
| Entry point | `py/main.py` | `cpp/src/main.cpp` |
| Tests | pytest (`py/tests/`) | GoogleTest + CTest (`cpp/test/`) |
| Build | none (`pyproject.toml` only configures pytest) | CMake + presets |

The same record, side by side:

```python
# py/todo/task.py
@dataclass
class Task:
    id: int
    title: str
    due_date: datetime | None = None
    priority: Priority = Priority.MEDIUM
    done: bool = False
```

```cpp
// cpp/include/task.h
struct Task {
  int id = 0;
  std::string title;
  Priority priority = Priority::Medium;
  bool done = false;
  std::string created_at;
  std::string due_date;  // empty string = no due date
};
```

> [!NOTE]
> Python's `None` has no single C++ equivalent, so the project uses `std::optional` where a value can
> be genuinely absent (`TaskManager::find`, `TaskManager::undo_remove`) and an empty string for the
> optional due date.

---

## Who this is for

- **Python developers learning C++** who want a familiar program to map new syntax onto
- **C++ developers** curious how the same design reads in Python
- **Students and self-taught developers** who want to see layered architecture in two languages
- **Anyone** wanting a compact reference for CMake + GoogleTest, or for stdlib-only Python + pytest

---

## What you can learn from it

- Data modelling with `@dataclass` / `IntEnum` vs `struct` / `enum class`
- Representing "maybe there is no value" with `None` vs `std::optional`
- JSON (de)serialization, plus tolerant reading of user-editable files
- Layered architecture and dependency injection without a framework
- Writing the same behaviour as a pytest suite and a GoogleTest suite
- A modern CMake setup: presets, `FetchContent`, warnings, sanitizers and `CTest`

---

## Where to start

Read one version top to bottom, following the layers, then read the other:

1. **Model** — [py/todo/task.py](py/todo/task.py) ↔ [cpp/include/task.h](cpp/include/task.h)
2. **Storage** — [py/todo/storage.py](py/todo/storage.py) ↔ [cpp/include/storage.h](cpp/include/storage.h)
3. **Business logic** — [py/todo/manager.py](py/todo/manager.py) ↔ [cpp/include/manager.h](cpp/include/manager.h)
4. **CLI** — [py/main.py](py/main.py) ↔ [cpp/src/main.cpp](cpp/src/main.cpp)
5. **Tests** — [py/tests/](py/tests) ↔ [cpp/test/](cpp/test)

Start with Python: it is shorter and needs no build step.

---

## Project layout

```text
.
├── py/                  # Python version — standard library only
│   ├── main.py          # CLI entry point
│   ├── todo/            # task.py (model), storage.py (JSON), manager.py (logic)
│   ├── tests/           # 109 pytest cases
│   └── data/tasks.json  # data file
└── cpp/                 # C++17 version
    ├── include/         # task.h, storage.h, manager.h
    ├── src/             # main.cpp, task.cpp, storage.cpp, manager.cpp
    ├── test/            # 67 GoogleTest cases (run with CTest)
    ├── CMakeLists.txt   # build, plus FetchContent for nlohmann/json and GoogleTest
    └── data/tasks.json  # data file
```

---

## Layered architecture

Both versions use the same three layers, with the same arrows:

```text
        main.py / main.cpp        ← CLI: shows the menu, reads input, prints results
                │
                │ calls
                ▼
           TaskManager            ← business logic: add / complete / remove / search / statistics
                │
                │ delegates persistence
                ▼
      JsonStorage / Storage       ← storage: reads and writes JSON
                │
                ▼
          data/tasks.json         ← your tasks, in plain JSON
```

Keeping the layers apart is what makes the two implementations easy to compare: every file has a
one-to-one counterpart in the other language (see [Where to start](#where-to-start)).

---

## Running the tests

**Python** — pytest is the only test dependency:

```bash
cd py
python -m pytest
```

There are 109 cases. Run this from inside `py/`; from the repository root use `python -m pytest py`.

**C++**:

```bash
cd cpp
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

There are 67 cases. Pass `-DBUILD_TESTING=OFF` to configure without them, and `cmake --preset asan`
instruments the tests with AddressSanitizer/UBSan.

---

## FAQ

**Which version should I read first?**
Python. It is shorter, has no build step, and the C++ version deliberately mirrors it.

**Are the two versions really the same?**
The same features, the same three layers and the same JSON shape. The C++ build adds one extra menu
item — undo the last delete (`8`). The [concept table](#python-vs-c-the-same-code-two-idioms) maps
how each idea is expressed.

**Do I need third-party libraries?**
Python needs none at runtime. C++ uses [nlohmann/json](https://github.com/nlohmann/json) for storage
and [GoogleTest](https://github.com/google/googletest) for tests; CMake uses an installed copy when
available and otherwise downloads the pinned release on the first configure, so the first build needs
network access.

**Why JSON instead of SQLite?**
The goal is to compare the two languages, not database APIs. JSON keeps the focus on data modelling,
serialization and error handling — and you can read or hand-edit the file.

**Can I use it as my everyday todo app?**
It works, but it is a teaching project: the interface is a plain numbered menu, with no sync or
reminders. Light, local use is fine.

**Can I reuse the code or the build files?**
Yes — it is [MIT licensed](LICENSE). The CMake + GoogleTest setup makes a decent starting template.

---

## Contributing

Issues and pull requests are welcome — especially fixes that keep the two versions equivalent, extra
tests, and documentation improvements. When you change behaviour, please update both versions.

---

## License

Released under the [MIT License](LICENSE).

---

<div align="center">

**[⬆ Back to top](#todo-manager)**

If this project helped you, a ⭐ helps other developers find it.

[**English**](README.md) · [简体中文](README.zh-CN.md)

</div>
