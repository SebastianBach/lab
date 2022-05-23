cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(laboratory)

add_executable(lab)
target_sources(lab PRIVATE src/lab.cpp)
target_include_directories(lab PUBLIC src)

