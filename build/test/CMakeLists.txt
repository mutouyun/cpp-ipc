cmake_minimum_required(VERSION 3.10)
project(test)

set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Test)

if(MSVC)
  add_compile_options(/std:c++17)
else()
  add_compile_options(-std=gnu++1z -Wno-unused-function -Wno-attributes)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

include_directories(../../include ../../src ../../test ../../test/capo)
file(GLOB SRC_FILES "../../test/*.cpp")
file(GLOB HEAD_FILES "../../test/*.h")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../output)

link_directories(${EXECUTABLE_OUTPUT_PATH})
add_subdirectory(../ipc ipc.out)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Test ipc)