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

FetchContent_Declare(googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.7.0
)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
add_definitions(-DBENCHMARK_STATIC_DEFINE)
FetchContent_MakeAvailable(googlebenchmark)

FetchContent_Declare(fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        9.1.0
)
FetchContent_MakeAvailable(fmtlib)

include_directories(
  ${LIBIPC_PROJECT_DIR}/include 
  ${LIBIPC_PROJECT_DIR}/src
  ${LIBIPC_PROJECT_DIR}/test)

file(GLOB SRC_FILES  ${LIBIPC_PROJECT_DIR}/benchmark/*.cpp)
file(GLOB HEAD_FILES ${LIBIPC_PROJECT_DIR}/benchmark/*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME}
  benchmark_main fmt
  imp pmr ipc)
