cmake_minimum_required(VERSION 3.10)
project(shadesmar)
SET(CMAKE_VERBOSE_MAKEFILE 1)

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
  list(APPEND libs rt pthread stdc++fs)# -fsanitize=thread -fsanitize=memory)
endif ()

add_compile_options(-O2)
#add_compile_options(-fsanitize=thread -fsanitize=memory -g -fno-omit-frame-pointer)
add_definitions(-DDEBUG_BUILD)
#add_definitions(-DOLD_LOCK)

add_executable(publish src/publisher.cpp)
target_link_libraries(publish ${libs})

add_executable(subscribe src/subscriber.cpp)
target_link_libraries(subscribe ${libs})

add_executable(flush_tmp src/flush_tmp.cpp)
target_link_libraries(flush_tmp ${libs})

add_executable(serialize_msg src/serialize_msg.cpp)
target_link_libraries(serialize_msg ${libs})

add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark ${libs})

add_executable(micro_bench src/micro_bench.cpp)
target_link_libraries(micro_bench ${libs})

add_executable(raw_benchmark src/raw_benchmark.cpp)
target_link_libraries(raw_benchmark ${libs})

add_executable(tmp_test src/tmp_test.cpp)
target_link_libraries(tmp_test ${libs})

add_executable(lock_tester src/lock_tester.cpp)
target_link_libraries(lock_tester ${libs})

add_executable(rpc_test src/rpc_test.cpp)
target_link_libraries(rpc_test ${libs})
