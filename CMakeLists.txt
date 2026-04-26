cmake_minimum_required(VERSION 3.15)
project(Renewal VERSION 1.0 LANGUAGES CXX)

# set C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
endif()

include(FetchContent)
option(RE2_ALLOW_FETCH "Fetch RE2 from source when no system package is available" ON)

find_package(RE2 QUIET)

if (NOT RE2_FOUND)
    if (NOT RE2_ALLOW_FETCH)
        message(FATAL_ERROR
            "RE2 was not found via find_package(RE2) and RE2_ALLOW_FETCH is OFF."
        )
    endif()

    message(STATUS "RE2 not found locally; fetching pinned RE2/Abseil sources")

    # Avoid update checks on already populated dependencies.
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

    # Minimize dependency build work in fallback mode.
    set(ABSL_BUILD_TESTING OFF CACHE BOOL "Disable Abseil tests" FORCE)
    set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "Disable Abseil install rules" FORCE)
    set(RE2_BUILD_TESTING OFF CACHE BOOL "Disable RE2 tests" FORCE)
    set(RE2_INSTALL OFF CACHE BOOL "Disable RE2 install/export rules" FORCE)

    # Use pinned upstream releases for reproducible builds.
    FetchContent_Declare(
        absl
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20250814.1
    )
    FetchContent_MakeAvailable(absl)

    FetchContent_Declare(
        re2
        GIT_REPOSITORY https://github.com/google/re2.git
        GIT_TAG 2025-11-05
    )
    FetchContent_MakeAvailable(re2)
endif()

# Include source subdirectory only
add_subdirectory(src)
