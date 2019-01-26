project(test-ipc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Test REQUIRED)

if(NOT MSVC)
  add_compile_options(-Wno-attributes -Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-function)
endif()

include_directories(../../include ../../src ../../test ../../test/capo)
file(GLOB SRC_FILES ../../test/*.cpp)
file(GLOB HEAD_FILES ../../test/*.h)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../output)

link_directories(${EXECUTABLE_OUTPUT_PATH})
add_subdirectory(../ipc ipc.out)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Test ipc)
if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} pthread rt)
endif()
