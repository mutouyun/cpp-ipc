project(ipc)

add_compile_options(-D__IPC_LIBRARY__)

if(NOT MSVC)
  add_compile_options(-fPIC)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src)

if(UNIX)
  file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/libipc/platform/*_linux.cpp)
else()
  file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/libipc/platform/*_win.cpp)
endif()
aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_FILES)

file(GLOB HEAD_FILES
  ${CMAKE_SOURCE_DIR}/include/libipc/*.h
  ${CMAKE_SOURCE_DIR}/src/libipc/*.h
  ${CMAKE_SOURCE_DIR}/src/libipc/*.inc
  ${CMAKE_SOURCE_DIR}/src/libipc/circ/*.h
  ${CMAKE_SOURCE_DIR}/src/libipc/memory/*.h
  ${CMAKE_SOURCE_DIR}/src/libipc/platform/*.h
  ${CMAKE_SOURCE_DIR}/src/libipc/utility/*.h)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES} ${HEAD_FILES})
if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
      pthread
      $<$<NOT:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>>:rt>)
endif()

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION "lib"
)
