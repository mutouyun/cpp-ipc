project(chat)

file(GLOB SRC_FILES ./*.cpp)
file(GLOB HEAD_FILES ./*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME} ipc)
