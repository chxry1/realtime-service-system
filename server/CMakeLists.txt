cmake_minimum_required(VERSION 3.10)
project(realtime_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    src/main.cpp
)
