cmake_minimum_required(VERSION 3.20)
project(clob VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLOB_ASAN "Enable AddressSanitizer" OFF)
option(CLOB_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()

add_library(clob
  src/book.cpp
  src/order.cpp
  src/price_level.cpp
  src/ladder.cpp
)

target_include_directories(clob PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(book_bench benchmarks/book_bench.cpp)
target_link_libraries(book_bench PRIVATE clob)

add_executable(clob_replay apps/clob_replay.cpp)
target_link_libraries(clob_replay PRIVATE clob)

function(clob_enable_sanitize target)
  if(NOT MSVC)
    if(CLOB_ASAN)
      target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address)
    endif()
    if(CLOB_UBSAN)
      target_compile_options(${target} PRIVATE -fsanitize=undefined)
      target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
  endif()
endfunction()

clob_enable_sanitize(clob)
clob_enable_sanitize(book_bench)
clob_enable_sanitize(clob_replay)

