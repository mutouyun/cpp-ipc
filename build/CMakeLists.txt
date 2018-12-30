cmake_minimum_required(VERSION 3.10)
project(ipc)

add_compile_definitions(__IPC_LIBRARY__)
if(MSVC)
  add_compile_options("/std:c++17")
else()
  add_compile_options("-std=gnu++1z -O2")
endif()

include_directories("../include")
include_directories("../src")

if(UNIX)
  file(GLOB DIR_SRCS ../src/platform/*_linux.cpp)
else()
  file(GLOB DIR_SRCS ../src/platform/*_win.cpp)
endif()
aux_source_directory(../src DIR_SRCS)

add_library(ipc SHARED ${DIR_SRCS})
set(LIBRARY_OUTPUT_PATH ../output)