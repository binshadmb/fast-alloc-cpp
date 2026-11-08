cmake_minimum_required(VERSION 3.16)
project(fast_alloc_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(fast_alloc src/fixed_block_pool.cpp)
target_include_directories(fast_alloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fast_alloc PUBLIC Threads::Threads)
target_compile_options(fast_alloc PRIVATE -Wall -Wextra -Wpedantic)

add_executable(test_fixed_block_pool tests/test_fixed_block_pool.cpp)
target_link_libraries(test_fixed_block_pool PRIVATE fast_alloc)

add_executable(benchmark_pool benchmarks/benchmark_pool.cpp)
target_link_libraries(benchmark_pool PRIVATE fast_alloc)

enable_testing()
add_test(NAME fixed_block_pool COMMAND test_fixed_block_pool)
