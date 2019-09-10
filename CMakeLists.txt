cmake_minimum_required(VERSION 3.10)
project(shadermar)
SET(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Boost)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread -lstdc++fs")
add_compile_options(-g)
include_directories(include)

add_executable(publish src/publisher.cpp)
target_link_libraries(publish ${Boost_LIBRARIES} rt stdc++fs)

add_executable(subscribe src/subscriber.cpp)
target_link_libraries(subscribe ${Boost_LIBRARIES} rt stdc++fs)

add_executable(flush_topic src/flush_topic.cpp)
target_link_libraries(flush_topic ${Boost_LIBRARIES} rt stdc++fs)

add_executable(tmp_test src/tmp_test.cpp)
target_link_libraries(tmp_test ${Boost_LIBRARIES} rt stdc++fs)