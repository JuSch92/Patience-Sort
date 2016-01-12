cmake_minimum_required(VERSION 2.5)
project(FinalPS)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -O3 -march=native")

set(SOURCE_FILES main.cpp)
add_executable(FinalPS ${SOURCE_FILES} PatienceSort.h RunPool.h)