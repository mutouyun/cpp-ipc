project(benchmark-ipc)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes 
    -Wno-missing-field-initializers 
    -Wno-unused-variable 
    -Wno-unused-function)
else()
  add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

include_directories(
  ${LIBIPC_PROJECT_DIR}/include 
  ${LIBIPC_PROJECT_DIR}/src
  ${LIBIPC_PROJECT_DIR}/test)

file(GLOB SRC_FILES  ${LIBIPC_PROJECT_DIR}/benchmark/*.cpp)
file(GLOB HEAD_FILES ${LIBIPC_PROJECT_DIR}/benchmark/*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME} benchmark_main imp ipc)
