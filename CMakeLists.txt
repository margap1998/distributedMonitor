cmake_minimum_required(VERSION 3.17)
project(untitled3)

add_subdirectory(monitor)
set(CMAKE_CXX_STANDARD 14)

add_executable(untitled3 examples/counter.cpp)

target_link_libraries(untitled3 monitor_zmq)