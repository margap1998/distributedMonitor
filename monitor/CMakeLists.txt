cmake_minimum_required(VERSION 3.17)
project(monitor_zmq)

set(CMAKE_CXX_STANDARD 14)
add_library(monitor_zmq monitor_message.h app_server.h app_server.cpp monitor_distributed.h)
target_link_libraries(monitor_zmq  zmq zmqpp pthread)