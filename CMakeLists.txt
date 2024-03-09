cmake_minimum_required(VERSION 3.11)
project(cpp-ipc)

option(LIBIPC_BUILD_TESTS       "Build all of libipc's own tests."                      OFF)
option(LIBIPC_BUILD_DEMOS       "Build all of libipc's own demos."                      OFF)
option(LIBIPC_BUILD_BENCHMARK   "Build all of libipc's own benchmark tests."            OFF)
option(LIBIPC_BUILD_SHARED_LIBS "Build shared libraries (DLLs)."                        OFF)
option(LIBIPC_USE_STATIC_CRT    "Set to ON to build with static CRT on Windows (/MT)."  OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
  set(CompilerFlags
      CMAKE_CXX_FLAGS
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_C_FLAGS
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE)
  if (LIBIPC_USE_STATIC_CRT)
      foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        string(REPLACE "/MDd" "/MTd" ${CompilerFlag} "${${CompilerFlag}}")
      endforeach()
  else()
      foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
        string(REPLACE "/MTd" "/MDd" ${CompilerFlag} "${${CompilerFlag}}")
      endforeach()
  endif()
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -rdynamic -fsanitize=address")
endif()

set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBIPC_PROJECT_DIR     ${PROJECT_SOURCE_DIR})

# FetchContent Support
include(FetchContent)
if(${CMAKE_VERSION} VERSION_LESS 3.14)
  macro(FetchContent_MakeAvailable NAME)
    FetchContent_GetProperties(${NAME})
    if(NOT ${NAME}_POPULATED)
      FetchContent_Populate(${NAME})
      add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
    endif()
  endmacro()
endif()

# Unicode Support
add_definitions(-DUNICODE -D_UNICODE)

add_subdirectory(src)

if (LIBIPC_BUILD_TESTS)
  add_subdirectory(test)
endif()

# if (LIBIPC_BUILD_DEMOS)
#   add_subdirectory(demo/chat)
#   add_subdirectory(demo/msg_que)
#   add_subdirectory(demo/send_recv)
# endif()

if (LIBIPC_BUILD_BENCHMARK)
  add_subdirectory(benchmark)
endif()

install(
  DIRECTORY "include/"
  DESTINATION "include"
)
