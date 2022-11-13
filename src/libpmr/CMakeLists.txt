project(pmr)

if(NOT MSVC)
  add_compile_options(
    -Wno-attributes)
endif()

aux_source_directory(${LIBIPC_PROJECT_DIR}/src/libpmr SRC_FILES)

file(GLOB HEAD_FILES
  ${LIBIPC_PROJECT_DIR}/include/libpmr/*.h)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${HEAD_FILES})

# set output directory
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(${PROJECT_NAME}
  PUBLIC  ${LIBIPC_PROJECT_DIR}/include
  PRIVATE ${LIBIPC_PROJECT_DIR}/src)

target_link_libraries(${PROJECT_NAME} PUBLIC imp)

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
