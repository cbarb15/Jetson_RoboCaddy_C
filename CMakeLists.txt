cmake_minimum_required(VERSION 3.21)
project(roboCaddy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(simpleble REQUIRED CONFIG)

add_executable(roboCaddy src/main.cpp)
target_link_libraries(roboCaddy PRIVATE simpleble::simpleble /usr/lib/libjetgpio.so)