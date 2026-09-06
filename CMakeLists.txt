cmake_minimum_required(VERSION 3.20)

project(EventBacktester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(event_backtester
    main.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(event_backtester PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
