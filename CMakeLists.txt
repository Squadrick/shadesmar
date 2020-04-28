cmake_minimum_required(VERSION 3.10)

project(shadesmar)

find_package(Boost REQUIRED)
find_package(msgpack REQUIRED)

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

add_compile_options(-march=native)
add_definitions(-DDEBUG_BUILD)

add_executable(flush_tmp src/flush_tmp.cpp)
target_link_libraries(flush_tmp ${libs})

# TESTS
function(TEST TARGET_NAME SRC_FILE)
  add_executable(${TARGET_NAME} ${SRC_FILE} test/catch_base.cpp)
  target_link_libraries(${TARGET_NAME} ${libs})
endfunction()

#TEST(pubsub_test test/pubsub_test.cpp)
#TEST(pubsub_bin_test test/pubsub_bin_test.cpp)
TEST(tmp_test test/tmp_test.cpp)
TEST(rpc_test test/rpc_test.cpp)

add_executable(micro_benchmark test/micro_benchmark.cpp)
target_link_libraries(micro_benchmark ${libs})