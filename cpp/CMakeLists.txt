# ===== Project settings =====
# This file needs CMake 3.16 at minimum.
# CMakePresets.json is an optional convenience and needs 3.21+; on older versions use the
# `cmake -S . -B build` form instead — identical behavior (see README).
cmake_minimum_required(VERSION 3.16)

project(todo
    VERSION 1.0.0
    DESCRIPTION "Terminal todo manager (C++ version)"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

option(TODO_ENABLE_SANITIZERS "Enable AddressSanitizer / UBSan (recommended only for Debug)" OFF)


# ===== Compiler flags shared by every target =====
# An INTERFACE target, so the warnings (and the optional sanitizers) are written once and
# inherited by the library, the app and the tests alike.
add_library(todo_flags INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(todo_flags INTERFACE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(todo_flags INTERFACE /W4 /permissive- /utf-8)
endif()

if(TODO_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(todo_flags INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(todo_flags INTERFACE -fsanitize=address,undefined)
    else()
        message(WARNING "TODO_ENABLE_SANITIZERS=ON currently supports GCC / Clang only; ignored.")
    endif()
endif()


# ===== Third-party: JSON =====
# Prefer an installed copy, otherwise fetch the pinned release — the same pattern as GoogleTest
# below. This replaces the header that used to be vendored under include/nlohmann/.
find_package(nlohmann_json 3.12.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found locally; fetching it with FetchContent")
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE
    )
    # Keep the dependency from dragging its own tests and install rules into this project.
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(nlohmann_json)
endif()


# ===== Core library =====
# Everything except main() lives here, so the app and the tests compile and link the logic
# once instead of each listing the same sources.
add_library(todo_core STATIC
    src/task.cpp
    src/manager.cpp
    src/storage.cpp
)

target_compile_features(todo_core PUBLIC cxx_std_17)
set_target_properties(todo_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(todo_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(todo_core PRIVATE
    include/manager.h
    include/storage.h
    include/task.h
)
# PUBLIC: the app and the tests inherit the warnings, sanitizers and the JSON header through
# this link.
target_link_libraries(todo_core PUBLIC todo_flags nlohmann_json::nlohmann_json)


# ===== Application =====
# main.cpp only talks to the user; it pulls in the logic through todo_core.
add_executable(todo src/main.cpp)
set_target_properties(todo PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(todo PRIVATE todo_core)

# The default data file path is baked in at compile time as an absolute path.
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/data/tasks.json" TODO_DATA_FILE_PATH)
target_compile_definitions(todo PRIVATE TODO_DATA_FILE="${TODO_DATA_FILE_PATH}")


# ===== Tests =====
# BUILD_TESTING is the conventional switch, so `-DBUILD_TESTING=OFF` skips the tests. The CTest
# module is deliberately not used: it would also add a pile of Nightly/Continuous dashboard
# targets this project has no use for, while `enable_testing()` alone is all `ctest` needs.
option(BUILD_TESTING "Build the unit tests (requires GoogleTest)" ON)

if(BUILD_TESTING)
    enable_testing()

    # Prefer an installed GoogleTest (for example the libgtest-dev package); fall back to
    # fetching it so a fresh checkout can still run the tests.
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GoogleTest not found locally; fetching it with FetchContent")
        include(FetchContent)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_subdirectory(test)
endif()
