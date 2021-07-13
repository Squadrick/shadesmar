cmake_minimum_required(VERSION 3.10)

project(shadesmar)


set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG_BUILD")

find_package(benchmark REQUIRED)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

if (UNIX AND NOT APPLE)
  list(APPEND libs rt pthread stdc++fs)
endif ()

add_compile_options(-march=native)
IF(SINGLE_H)
  add_definitions(-DSINGLE_HEADER)
ENDIF(SINGLE_H)


add_executable(flush_tmp src/flush_tmp.cpp)
target_link_libraries(flush_tmp ${libs})

# TESTS

add_executable(pubsub_bin_test test/pubsub_bin_test.cpp)
target_link_libraries(pubsub_bin_test ${libs})

add_executable(micro_benchmark test/micro_benchmark.cpp)
target_link_libraries(micro_benchmark ${libs})

add_executable(tmp_test test/tmp_test.cpp)
target_link_libraries(tmp_test ${libs})

add_executable(dragons_test test/dragons_test.cpp)
target_link_libraries(dragons_test ${libs})

add_executable(allocator_test test/allocator_test.cpp)
target_link_libraries(allocator_test ${libs})

# BENCHMARKS
add_executable(dragons_bench benchmark/dragons.cpp)
target_link_libraries(dragons_bench ${libs} benchmark::benchmark)
