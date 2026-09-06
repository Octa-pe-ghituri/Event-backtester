cmake_minimum_required(VERSION 3.20)

project(EventBacktester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(event_backtester
    main.cpp
    src/backtest.cpp
    src/book.cpp
    src/io.cpp
    src/portfolio.cpp
    src/fair_price_strategy.cpp
)

target_include_directories(event_backtester PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    target_compile_options(event_backtester PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )

    if(ENABLE_SANITIZERS)

        target_compile_options(event_backtester PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(event_backtester PRIVATE
            -fsanitize=address,undefined
        )

    endif()

endif()
