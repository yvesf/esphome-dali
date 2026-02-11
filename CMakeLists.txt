cmake_minimum_required(VERSION 3.15...4.1)
project(dali
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wall -Wextra -Werror -Wpedantic")

option(USE_TIDY "use clang-tidy" OFF)
if(USE_TIDY)
    message("-- clang-tidy enabled")
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
else()
    message("-- clang-tidy disabled")
endif(USE_TIDY)
unset(USE_TIDY CACHE)

add_executable(dali)
target_sources(dali
  PRIVATE
    src/main.cpp
    src/linuxi2c.cpp
    components/dali/lw14.cpp
  PUBLIC
  FILE_SET header
  TYPE HEADERS
  BASE_DIRS
    components/dali
    src
  FILES
    components/dali/dali.h
    components/dali/lw14.h
    src/linuxi2c.h
)

find_package(Catch2 3 REQUIRED)
file(GLOB Testfiles
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    Testing/test_*.cpp
)
add_executable(tests ${Testfiles})
target_sources(tests
  PUBLIC
  FILE_SET header
  TYPE HEADERS
  BASE_DIRS
    components/dali
    src
  FILES
    components/dali/dali.h
    components/dali/lw14.h
    src/linuxi2c.h
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)

# enable_testing()

# file(GLOB Testfiles
#     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
#     Testing/test_*.cpp
# )
# create_test_sourcelist(tests
#     Testing/Tests.cpp
#     ${Testfiles}
# )
# add_executable(tests ${tests})
# target_sources(tests
#   PUBLIC
#   FILE_SET header
#   TYPE HEADERS
#   BASE_DIRS
#     components/dali
#     src
#   FILES
#     components/dali/dali.h
#     components/dali/lw14.h
#     src/linuxi2c.h
# )
# target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
# foreach(test ${Testfiles})
# get_filename_component(TName ${test} NAME_WE)
# add_test(NAME Testing/${TName} COMMAND tests Testing/${TName})
# endforeach()
