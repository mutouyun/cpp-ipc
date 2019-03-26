project(chat)

include_directories(../../include)
file(GLOB SRC_FILES ../../demo/chat/*.cpp)
file(GLOB HEAD_FILES ../../demo/chat/*.h)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../output)

link_directories(${EXECUTABLE_OUTPUT_PATH})
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME} ipc)
if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} pthread rt)
endif()
