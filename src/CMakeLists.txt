project(ipc)

option(LIBIPC_BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)

if(NOT MSVC)
  add_compile_options(-fPIC)
endif()

include_directories(
    ${LIBIPC_PROJECT_DIR}/include 
    ${LIBIPC_PROJECT_DIR}/src)

if(UNIX)
  file(GLOB SRC_FILES ${LIBIPC_PROJECT_DIR}/src/libipc/platform/*_linux.cpp)
else()
  file(GLOB SRC_FILES ${LIBIPC_PROJECT_DIR}/src/libipc/platform/*_win.cpp)
endif()
aux_source_directory(${LIBIPC_PROJECT_DIR}/src SRC_FILES)

file(GLOB HEAD_FILES
  ${LIBIPC_PROJECT_DIR}/include/libipc/*.h
  ${LIBIPC_PROJECT_DIR}/src/libipc/*.h
  ${LIBIPC_PROJECT_DIR}/src/libipc/*.inc
  ${LIBIPC_PROJECT_DIR}/src/libipc/circ/*.h
  ${LIBIPC_PROJECT_DIR}/src/libipc/memory/*.h
  ${LIBIPC_PROJECT_DIR}/src/libipc/platform/*.h
  ${LIBIPC_PROJECT_DIR}/src/libipc/utility/*.h)

if (LIBIPC_BUILD_SHARED_LIBS)
  add_library(${PROJECT_NAME} SHARED ${SRC_FILES} ${HEAD_FILES})
  target_compile_definitions(${PROJECT_NAME}
    INTERFACE
      LIBIPC_LIBRARY_SHARED_USING__
    PRIVATE
      LIBIPC_LIBRARY_SHARED_BUILDING__)
else()
  add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${HEAD_FILES})
endif()

if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    pthread
    $<$<NOT:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>>:rt>)
endif()

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION "lib"
)
