cmake_minimum_required(VERSION 3.16)

project(process_monitor
    VERSION 0.1.0
    DESCRIPTION "Console process monitoring utility for Linux"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(process_monitor
    src/main.cpp
    src/process_reader.cpp
    src/process_manager.cpp
    src/ui.cpp
)

target_include_directories(process_monitor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(process_monitor PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
elseif (MSVC)
    target_compile_options(process_monitor PRIVATE /W4)
endif()