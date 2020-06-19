cmake_minimum_required(VERSION 3.10)

project(shadesmar)

find_package(Boost REQUIRED)
find_package(msgpack REQUIRED)
find_package(benchmark REQUIRED)

set(CMAKE_CXX_STANDARD 17)

if (Boost_FOUND)
  set(libs ${Boost_LIBRARIES})
  include_directories(include ${Boost_INCLUDE_DIRS})
endif ()

if (APPLE)
  if ("${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "10.14")
    set(CMAKE_CXX_STANDARD 11)
  endif ()
endif ()

if (UNIX AND NOT APPLE)
  list(APPEND libs rt pthread stdc++fs)
endif ()

add_compile_options(-mavx -mavx2 -march=native -O2)
add_definitions(-DDEBUG_BUILD)

add_executable(flush_tmp src/flush_tmp.cpp)
target_link_libraries(flush_tmp ${libs})

# TESTS
add_executable(pubsub_test test/pubsub_test.cpp)
target_link_libraries(pubsub_test ${libs})

add_executable(pubsub_bin_test test/pubsub_bin_test.cpp)
target_link_libraries(pubsub_bin_test ${libs})

add_executable(micro_benchmark test/micro_benchmark.cpp)
target_link_libraries(micro_benchmark ${libs})

add_executable(tmp_test test/tmp_test.cpp)
target_link_libraries(tmp_test ${libs})

add_executable(rpc_test test/rpc_test.cpp)
target_link_libraries(rpc_test ${libs})

add_executable(dragons_test test/dragons_test.cpp)
target_link_libraries(dragons_test ${libs})

# BENCHMARKS
add_executable(dragons_bench benchmark/dragons.cpp)
target_link_libraries(dragons_bench ${libs} benchmark::benchmark)
