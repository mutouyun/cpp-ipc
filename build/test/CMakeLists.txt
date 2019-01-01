project(test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH /usr/include/qt5)

find_package(Qt5 COMPONENTS Core Test REQUIRED)

if(NOT MSVC)
  add_compile_options(-Wno-unused-function -Wno-attributes)
endif()

include_directories(../../include ../../src ../../test ../../test/capo)
file(GLOB SRC_FILES "../../test/*.cpp")
file(GLOB HEAD_FILES "../../test/*.h")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../output)

link_directories(${EXECUTABLE_OUTPUT_PATH})
add_subdirectory(../ipc ipc.out)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Test ipc)