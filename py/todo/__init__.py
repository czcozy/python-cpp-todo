"""The todo package: keeps the parts of the todo manager together.

What __init__.py is for:
1. It tells Python "the todo folder is a package that can be imported".
2. It also re-exports the common names up front, so outside code can write:
       from todo import TaskManager, Priority
   instead of the long form from todo.manager import TaskManager.
"""

from .manager import TaskManager
from .storage import JsonStorage
from .task import TIME_FORMAT, Priority, Task

# __all__ is the list of names exported by "from todo import *"; a good habit.
__all__ = ["TIME_FORMAT", "JsonStorage", "Priority", "Task", "TaskManager"]
