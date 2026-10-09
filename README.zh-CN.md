<div align="center">

# Todo Manager 待办清单

**同一个命令行待办工具，用两种语言各写一遍 —— 纯 Python 🐍 与 现代 C++17 ⚙️**

<sub>功能相同 · 分层相同 · JSON 相同，变的只是语言习惯</sub>

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](cpp/)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10%2B-3776AB?logo=python&logoColor=white)](py/)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)](cpp/CMakeLists.txt)
[![pytest — 109 个用例](https://img.shields.io/badge/pytest-109%20cases-brightgreen)](py/tests/)
[![GoogleTest — 67 个用例](https://img.shields.io/badge/GoogleTest-67%20cases-brightgreen)](cpp/test/)
[![PRs welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)](#参与贡献)
[![MIT license](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

[English](README.md) · [**简体中文**](README.zh-CN.md)

</div>

这是一个终端 **待办清单管理器**，用 **Python** 与 **C++17** 各实现了一遍：功能一致、分层架构一致、
JSON 数据格式一致，只有写法不同。因此它是一份小巧、易读的 **Python 与 C++ 对照学习项目**，可以
把同一份设计在两种语言里并排阅读：`@dataclass` ↔ `struct`、`None` ↔ `std::optional`、
`IntEnum` ↔ `enum class`、pytest ↔ GoogleTest。

建议先一口气读完简短的 Python 版本，再读 C++ 版本，看同样的思路如何在另一种语言中对应起来。

---

## 目录

- [演示](#演示)
- [功能特性](#功能特性)
- [快速开始](#快速开始)
- [Python 与 C++ 对照](#python-与-c-对照)
- [适合谁读](#适合谁读)
- [你能学到什么](#你能学到什么)
- [从哪里开始](#从哪里开始)
- [项目结构](#项目结构)
- [分层架构](#分层架构)
- [运行测试](#运行测试)
- [常见问题](#常见问题)
- [参与贡献](#参与贡献)
- [许可证](#许可证)

---

## 演示

一段精简的会话（每次操作后都会重新打印菜单）：

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
Task title: 买牛奶
Priority 1=low 2=medium 3=high (Enter=medium): 2
Due date (format 2026-10-05 18:00, Enter=none): 2026-10-10 09:00
Added #1
Choose > 1
Task title: 写文档
Priority 1=low 2=medium 3=high (Enter=medium): 3
Due date (format 2026-10-05 18:00, Enter=none):
Added #2
Choose > 6
Total 2: 0 done, 2 pending
Choose > 2
  ⬜ #1   !    "买牛奶" [medium] (2026-10-09 12:22)(2026-10-10 09:00)
  ⬜ #2   !!!  "写文档" [high] (2026-10-09 12:22)(no due)
Choose > 0
Bye!
```

> [!NOTE]
> 程序界面是英文的，但任务标题支持中文，数据也以 UTF-8 原样写入 JSON。C++ 版本菜单相同，只是多了
> **8. Undo delete**（撤销上一次删除）。

---

## 功能特性

两个版本的功能完全一致：

- ➕ 在编号菜单里添加、📋 查看、✅ 完成、🗑️ 删除、🔍 搜索任务
- 🎚️ **优先级**（低 / 中 / 高）与 📅 **可选截止时间**
- 📊 **统计**：总数、已完成、未完成（菜单 `6. Show statistics`）
- 💾 **JSON 持久化**，重启后数据仍在
- 🛡️ **容错保护**：手工改坏的或格式错误的数据文件会被备份而不是丢失，无法解析的条目会被跳过
- 🧱 **分层架构**：数据模型、存储、业务逻辑与命令行界面彼此独立
- 🔌 **依赖注入**：由 `main` 把存储交给管理器，两层互不拥有对方
- 🐍 **Python 零依赖**：只用标准库，运行时不需要任何第三方包
- 🧪 两侧都有测试：**109 个 pytest 用例** 与 **67 个 GoogleTest 用例**

仅在 C++ 版本中：

- ↩️ **撤销上一次删除**（菜单 `8. Undo delete`）

---

## 快速开始

**Python** —— 需要 Python 3.10+，无需第三方包：

```bash
cd py
python3 main.py
```

**C++** —— 需要 C++17 编译器、CMake 与 Ninja：

```bash
cd cpp
cmake --preset default
cmake --build --preset default
./build/todo
```

> [!TIP]
> 两个版本都把第一个命令行参数当作数据文件路径：
> `python3 main.py /tmp/my_tasks.json` 或 `./build/todo /tmp/my_tasks.json`。

两者都会打印上面的编号菜单：输入编号执行操作，输入 `0` 退出。数据默认写在各自目录下的
`data/tasks.json`。C++ 的 preset 需要 CMake 3.21+；更老的 CMake 请改用 `cmake -S . -B build` 与
`cmake --build build`。

更详细的说明（包括如何运行测试）见 [`py/README.zh-CN.md`](py/README.zh-CN.md) 与
[`cpp/README.zh-CN.md`](cpp/README.zh-CN.md)。

---

## Python 与 C++ 对照

| 概念 | Python | C++17 |
| --- | --- | --- |
| 一条记录 | `@dataclass Task` | `struct Task` |
| 优先级常量 | `IntEnum Priority` | `enum class Priority` |
| 集合 | `list[Task]` | `std::vector<Task>` |
| 可能没有值 | `None` | `std::optional<T>` |
| 模块 / 命名空间 | 包 `todo` | `namespace todo` |
| 存储 | `JsonStorage`（JSON） | `Storage`（JSON） |
| 业务逻辑 | `TaskManager` | `TaskManager` |
| 入口 | `py/main.py` | `cpp/src/main.cpp` |
| 测试 | pytest（`py/tests/`） | GoogleTest + CTest（`cpp/test/`） |
| 构建 | 无（`pyproject.toml` 只配置 pytest） | CMake + preset |

同一条记录，并排对照：

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
  std::string due_date;  // 空字符串表示没有截止时间
};
```

> [!NOTE]
> Python 的 `None` 在 C++ 里没有单一对应物：本项目在「值可能真的不存在」的地方用 `std::optional`
> （`TaskManager::find`、`TaskManager::undo_remove`），可选的截止时间则用空字符串表示。

---

## 适合谁读

- **从 Python 转向 C++ 的开发者**：想拿一个熟悉的程序来对照新语法
- **C++ 开发者**：想看看同一套设计用 Python 写出来是什么样
- **学生与自学者**：想在两种语言里看到同一个分层架构
- **想找模板的人**：一份精简的 CMake + GoogleTest 参考，或纯标准库 Python + pytest 参考

---

## 你能学到什么

- 用 `@dataclass` / `IntEnum` 与 `struct` / `enum class` 给数据建模
- 用 `None` 与 `std::optional` 表达「可能没有值」
- JSON 序列化与反序列化，以及如何宽容地读取用户可能手改过的文件
- 不用框架实现分层架构与依赖注入
- 把同一套行为分别写成 pytest 测试与 GoogleTest 测试
- 现代 CMake 用法：preset、`FetchContent`、编译警告、Sanitizer 与 `CTest`

---

## 从哪里开始

先自上而下读完一个版本（按分层顺序），再读另一个版本：

1. **模型** —— [py/todo/task.py](py/todo/task.py) ↔ [cpp/include/task.h](cpp/include/task.h)
2. **存储** —— [py/todo/storage.py](py/todo/storage.py) ↔ [cpp/include/storage.h](cpp/include/storage.h)
3. **业务逻辑** —— [py/todo/manager.py](py/todo/manager.py) ↔ [cpp/include/manager.h](cpp/include/manager.h)
4. **界面** —— [py/main.py](py/main.py) ↔ [cpp/src/main.cpp](cpp/src/main.cpp)
5. **测试** —— [py/tests/](py/tests) ↔ [cpp/test/](cpp/test)

建议从 Python 开始：代码更短，而且不需要构建。

---

## 项目结构

```text
.
├── py/                  # Python 版本 —— 只用标准库
│   ├── main.py          # 命令行入口
│   ├── todo/            # task.py（模型）、storage.py（JSON）、manager.py（逻辑）
│   ├── tests/           # 109 个 pytest 用例
│   └── data/tasks.json  # 数据文件
└── cpp/                 # C++17 版本
    ├── include/         # task.h、storage.h、manager.h
    ├── src/             # main.cpp、task.cpp、storage.cpp、manager.cpp
    ├── test/            # 67 个 GoogleTest 用例（用 CTest 运行）
    ├── CMakeLists.txt   # 构建，并用 FetchContent 拉取 nlohmann/json 与 GoogleTest
    └── data/tasks.json  # 数据文件
```

---

## 分层架构

两个版本使用相同的三层，箭头也相同：

```text
        main.py / main.cpp        ← 界面层：打印菜单、读取输入、输出结果
                │
                │ 调用
                ▼
           TaskManager            ← 业务逻辑：增删改查、搜索、统计
                │
                │ 委托持久化
                ▼
      JsonStorage / Storage       ← 存储层：读写 JSON
                │
                ▼
          data/tasks.json         ← 你的任务，纯 JSON
```

正是这种分层让两种实现便于对照：每个文件在另一种语言里都有一一对应的文件
（见 [从哪里开始](#从哪里开始)）。

---

## 运行测试

**Python** —— pytest 是唯一的测试依赖：

```bash
cd py
python -m pytest
```

共 109 个用例。请在 `py/` 目录内运行；如果在仓库根目录，请用 `python -m pytest py`。

**C++**：

```bash
cd cpp
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

共 67 个用例。配置时加 `-DBUILD_TESTING=OFF` 可跳过测试；`cmake --preset asan` 会用
AddressSanitizer/UBSan 插桩测试。

---

## 常见问题

**应该先读哪个版本？**
先读 Python。它更短、不需要构建，而且 C++ 版本是刻意照着它写的。

**两个版本真的完全一样吗？**
功能、三层结构与 JSON 结构都相同。C++ 版本多一个菜单项 —— 撤销上一次删除（`8`）。各概念如何
对应，见 [对照表](#python-与-c-对照)。

**需要第三方库吗？**
Python 运行时不需要任何第三方库。C++ 用 [nlohmann/json](https://github.com/nlohmann/json) 做存储、
用 [GoogleTest](https://github.com/google/googletest) 做测试；CMake 会优先使用系统里已安装的版本，
否则在首次 configure 时下载固定版本，因此第一次构建需要联网。

**为什么用 JSON 而不是 SQLite？**
这个项目的目标是比较两种语言，而不是比较数据库 API。JSON 让重点留在数据建模、序列化与错误处理
上，而且文件可以直接阅读、手工编辑。

**可以当作日常待办工具用吗？**
可以用，但它是教学项目：界面就是朴素的编号菜单，没有同步与提醒。本地轻量使用没问题。

**可以直接复用代码或构建脚本吗？**
可以，项目采用 [MIT 许可证](LICENSE)。其中的 CMake + GoogleTest 配置适合当作起步模板。

---

## 参与贡献

欢迎提交 Issue 与 Pull Request —— 尤其是能让两个版本保持一致、补充测试、改进文档的改动。改动
行为时，请同步更新两个版本。

---

## 许可证

基于 [MIT License](LICENSE) 发布。

---

<div align="center">

**[⬆ 回到顶部](#todo-manager-待办清单)**

如果你觉得这个项目有帮助，点一个 ⭐ 能让更多开发者看到它。

[English](README.md) · [**简体中文**](README.zh-CN.md)

</div>
