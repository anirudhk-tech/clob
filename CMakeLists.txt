cmake_minimum_required(VERSION 3.20)
project(clob VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /experimental:module)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()

# LSP compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main clob library
add_library(clob
    src/book.cpp
    src/order.cpp
    src/price_level.cpp
)

target_include_directories(clob PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(clob PUBLIC cxx_std_20)

# Benchmarks
add_executable(book_bench benchmarks/book_bench.cpp)
target_link_libraries(book_bench PRIVATE clob)
target_compile_features(book_bench PUBLIC cxx_std_20)

# Replay app
add_executable(clob_replay apps/clob_replay.cpp)
target_link_libraries(clob_replay PRIVATE clob)
target_compile_features(clob_replay PUBLIC cxx_std_20)

# Tests
enable_testing()
add_subdirectory(tests)

