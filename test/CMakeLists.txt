project(test-ipc)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes 
    -Wno-missing-field-initializers 
    -Wno-unused-variable 
    -Wno-unused-function)
endif()

include_directories(
    ${LIBIPC_PROJECT_DIR}/include 
    ${LIBIPC_PROJECT_DIR}/src
    ${LIBIPC_PROJECT_DIR}/test
    ${LIBIPC_PROJECT_DIR}/3rdparty
    ${LIBIPC_PROJECT_DIR}/3rdparty/gtest/include)

# Collect only new test files (exclude archive directory)
file(GLOB SRC_FILES
    ${LIBIPC_PROJECT_DIR}/test/test_*.cpp
    ${LIBIPC_PROJECT_DIR}/test/imp/*.cpp
    ${LIBIPC_PROJECT_DIR}/test/mem/*.cpp
    # ${LIBIPC_PROJECT_DIR}/test/profiler/*.cpp
    )
file(GLOB HEAD_FILES ${LIBIPC_PROJECT_DIR}/test/test_*.h)

# Ensure we don't include archived tests
list(FILTER SRC_FILES EXCLUDE REGEX "archive")
list(FILTER HEAD_FILES EXCLUDE REGEX "archive")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

link_directories(${LIBIPC_PROJECT_DIR}/3rdparty/gperftools)
target_link_libraries(${PROJECT_NAME} gtest gtest_main ipc)
#target_link_libraries(${PROJECT_NAME} tcmalloc_minimal)
