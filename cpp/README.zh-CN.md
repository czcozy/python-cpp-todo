# Todo Manager（C++ 版）

[English](README.md) · [简体中文](README.zh-CN.md)

[Python × C++ Todo Manager](../README.zh-CN.md) 对照项目的一部分。

用 C++17 编写的终端待办管理器。功能与 [`../py`](../py/README.zh-CN.md) 版本一致 —— 添加、查看、
完成、删除、搜索任务，并支持优先级与截止时间 —— 另外多出 **撤销上一次删除**
（`8. Undo delete`），这是 Python 版本没有的。数据保存在 `data/tasks.json`，重启后依然存在。

## 构建与运行

```bash
cd cpp
cmake --preset default        # 生成构建文件（只需一次）；输出到 build/
cmake --build --preset default
./build/todo                  # 运行可执行文件
```

preset 定义在 `CMakePresets.json` 中，需要 CMake 3.21+。使用更老的 CMake 时，改用下面等价的
原生命令：

```bash
cmake -S . -B build
cmake --build build
```

preset 使用的生成器是 Ninja，因此必须安装 `ninja`。程序默认读写 `data/tasks.json`；也可以指定
其他文件：

```bash
./build/todo                # 使用默认的 data/tasks.json
./build/todo /tmp/xx.json   # 使用其他文件
```

## 运行测试

单元测试位于 `test/`，使用 GoogleTest。它们与应用由同一条命令一起构建（默认开启
`BUILD_TESTING`），并用 CTest 运行：

```bash
cd cpp
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

配置时加 `-DBUILD_TESTING=OFF` 可跳过测试；`cmake --preset asan` 预设同样会对测试做插桩。

共 67 个用例。

## 依赖

其余部分都来自标准库。两个第三方组件是
[nlohmann/json](https://github.com/nlohmann/json)（存储）与
[GoogleTest](https://github.com/google/googletest)（测试）。CMake 会优先使用系统里已安装的版本
（`nlohmann-json3-dev`、`libgtest-dev`），否则在首次 configure 时拉取固定版本，因此需要联网。

---

[English](README.md) · [简体中文](README.zh-CN.md) · [↑ 返回项目总览](../README.zh-CN.md)
