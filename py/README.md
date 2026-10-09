# Todo Manager (Python version)

[English](README.md) · [简体中文](README.zh-CN.md)

Part of the [Python × C++ Todo Manager](../README.md) comparison project.

A terminal todo manager: add, view, complete and delete tasks, with priorities, due dates and
search. Data is stored in `data/tasks.json`, so tasks survive a restart. The business code uses only
the Python standard library — there are no runtime dependencies.

> [!NOTE]
> The C++ version adds one menu item this version does not have — undo the last delete (`8`).

## Setup

Python 3.10 or newer is required; running the tests also needs pytest. Create an isolated
environment with conda (once):

```bash
conda create -n todo python=3.12 pytest -y
conda activate todo
```

Any other way of installing Python and pytest works too — the program itself needs no third-party
packages.

## Run

```bash
cd py
python main.py
```

Then pick a function by its menu number (`0` to exit).

You can also point it at another data file without touching the default `data/tasks.json`:

```bash
python main.py /tmp/my_tasks.json
```

## Run the tests

```bash
cd py
python -m pytest
```

There are 109 cases. Run this from inside `py/`; from the repository root use `python -m pytest py`.

---

[English](README.md) · [简体中文](README.zh-CN.md) · [↑ Back to the project overview](../README.md)
