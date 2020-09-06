project(ipc)

add_compile_options(-D__IPC_LIBRARY__)

include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src
)

if(UNIX)
  file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/platform/*_linux.cpp)
else()
  file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/platform/*_win.cpp)
endif()
aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_FILES)
file(GLOB HEAD_FILES ${CMAKE_SOURCE_DIR}/include/*.h ${CMAKE_SOURCE_DIR}/src/*.inc ${CMAKE_SOURCE_DIR}/src/memory/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES} ${HEAD_FILES})
