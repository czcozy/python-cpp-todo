# The tests link against the same todo_core library the app uses, so there is no second copy
# of the logic to keep in sync.
add_executable(todo_tests
    test_priority.cpp
    test_task.cpp
    test_storage.cpp
    test_manager.cpp
)

set_target_properties(todo_tests PROPERTIES CXX_EXTENSIONS OFF)

# todo_core already brings C++17, the include directory and the warning/sanitizer flags along.
target_link_libraries(todo_tests PRIVATE todo_core GTest::gtest_main)

# Register every test case with CTest individually, so `ctest -R <name>` works and the
# editor's test explorer lists them one by one.
include(GoogleTest)
gtest_discover_tests(todo_tests)
