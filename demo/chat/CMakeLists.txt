project(chat)

if (MSVC AND USE_STATIC_CRT)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

file(GLOB SRC_FILES ./*.cpp)
file(GLOB HEAD_FILES ./*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME} ipc)
