# Todo Manager (C++ version)

[English](README.md) · [简体中文](README.zh-CN.md)

Part of the [Python × C++ Todo Manager](../README.md) comparison project.

A terminal todo manager written in C++17. It shares its features with the
[`../py`](../py/README.md) version — add, view, complete, delete and search tasks, with priorities
and due dates — and adds **undo the last delete** (`8. Undo delete`), which the Python version does
not have. Data is stored in `data/tasks.json`, so tasks survive a restart.

## Build and run

```bash
cd cpp
cmake --preset default        # generate build files (only needed once); output goes to build/
cmake --build --preset default
./build/todo                  # run the executable
```

The presets are defined in `CMakePresets.json` and require CMake 3.21+. With an older CMake, use the
plain commands instead (they are equivalent):

```bash
cmake -S . -B build
cmake --build build
```

The preset generator is Ninja, so `ninja` must be installed. The program reads and writes
`data/tasks.json` by default; you can point it at another file:

```bash
./build/todo                # use the default data/tasks.json
./build/todo /tmp/xx.json   # use another file
```

## Run the tests

The unit tests live in `test/` and use GoogleTest. They are built by the same command as the app
(`BUILD_TESTING` is on by default) and run with CTest:

```bash
cd cpp
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

Pass `-DBUILD_TESTING=OFF` to configure without the tests; the sanitizer preset
(`cmake --preset asan`) instruments the tests as well.

## Dependencies

The rest is standard library. The two third-party pieces are
[nlohmann/json](https://github.com/nlohmann/json) (storage) and
[GoogleTest](https://github.com/google/googletest) (tests). CMake uses an installed copy when there
is one (`nlohmann-json3-dev`, `libgtest-dev`) and otherwise fetches the pinned release on the first
configure, which therefore needs network access.

---

[English](README.md) · [简体中文](README.zh-CN.md) · [↑ Back to the project overview](../README.md)
