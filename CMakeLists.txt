cmake_minimum_required(VERSION 3.9)
project(cpp-ipc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)

set(GOOGLETEST_VERSION 1.10.0)
add_subdirectory(3rdparty/gtest)
add_subdirectory(test)

add_subdirectory(demo/chat)
add_subdirectory(demo/msg_que)
add_subdirectory(demo/kvs)
add_subdirectory(demo/PyCpp)
