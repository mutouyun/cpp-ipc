project(test-ipc)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes 
    -Wno-missing-field-initializers 
    -Wno-unused-variable 
    -Wno-unused-function)
endif()

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.12.0
)
if (LIBIPC_USE_STATIC_CRT)
  set(gtest_force_shared_crt OFF)
else()
  set(gtest_force_shared_crt ON)
endif()
FetchContent_MakeAvailable(googletest)

include_directories(
  ${LIBIPC_PROJECT_DIR}/include 
  ${LIBIPC_PROJECT_DIR}/src
  ${LIBIPC_PROJECT_DIR}/test)

file(GLOB SRC_FILES
  ${LIBIPC_PROJECT_DIR}/test/imp/*.cpp
  ${LIBIPC_PROJECT_DIR}/test/pmr/*.cpp
  ${LIBIPC_PROJECT_DIR}/test/concur/*.cpp
  ${LIBIPC_PROJECT_DIR}/test/*.cpp)

file(GLOB HEAD_FILES
  ${LIBIPC_PROJECT_DIR}/test/*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

target_link_libraries(${PROJECT_NAME} gtest_main imp pmr ipc)
