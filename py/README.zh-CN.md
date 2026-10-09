# Todo Manager（Python 版）

[English](README.md) · [简体中文](README.zh-CN.md)

[Python × C++ Todo Manager](../README.zh-CN.md) 对照项目的一部分。

一个终端待办管理器：添加、查看、完成、删除任务，并支持优先级、截止时间与搜索。数据保存在
`data/tasks.json`，重启后依然存在。业务代码只使用 Python 标准库，运行时没有任何第三方依赖。

> [!NOTE]
> C++ 版本比本版本多一个菜单项 —— 撤销上一次删除（`8. Undo delete`）。

## 环境准备

需要 Python 3.10 或更高版本；运行测试还需要 pytest。用 conda 创建独立环境（只需一次）：

```bash
conda create -n todo python=3.12 pytest -y
conda activate todo
```

其他安装 Python 与 pytest 的方式同样可行 —— 程序本身不需要任何第三方包。

## 运行

```bash
cd py
python main.py
```

然后按菜单编号选择功能（输入 `0` 退出）。

也可以指定其他数据文件，而不动默认的 `data/tasks.json`：

```bash
python main.py /tmp/my_tasks.json
```

## 运行测试

```bash
cd py
python -m pytest
```

共 109 个用例。请在 `py/` 目录内运行；如果在仓库根目录，请用 `python -m pytest py`。

---

[English](README.md) · [简体中文](README.zh-CN.md) · [↑ 返回项目总览](../README.zh-CN.md)
