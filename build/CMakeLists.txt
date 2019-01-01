cmake_minimum_required(VERSION 3.10)
project(cpp-ipc)

set(CMAKE_CXX_STANDARD 17)
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

add_subdirectory(test)