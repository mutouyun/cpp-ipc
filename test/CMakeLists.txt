project(test-ipc)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes 
    -Wno-missing-field-initializers 
    -Wno-unused-variable 
    -Wno-unused-function)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/3rdparty
    ${CMAKE_SOURCE_DIR}/3rdparty/gtest/include
)

file(GLOB SRC_FILES  ${CMAKE_SOURCE_DIR}/test/*.cpp)
file(GLOB HEAD_FILES ${CMAKE_SOURCE_DIR}/test/*.h)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES})

link_directories(${CMAKE_SOURCE_DIR}/3rdparty/gperftools)
target_link_libraries(${PROJECT_NAME} gtest gtest_main ipc)
#target_link_libraries(${PROJECT_NAME} tcmalloc_minimal)
if(NOT MSVC)
  target_link_libraries(${PROJECT_NAME} pthread rt)
endif()
