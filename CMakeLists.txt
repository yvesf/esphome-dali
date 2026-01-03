cmake_minimum_required(VERSION 3.15...4.1)
project(
  dali
  VERSION 1.0
  LANGUAGES CXX)

add_executable(dali)
include_directories(components/dali)
target_sources(dali
  PRIVATE
    src/main.cpp
    src/linuxi2c.cpp
    components/dali/lw14.cpp

  PUBLIC
  FILE_SET header
  TYPE HEADERS
  FILES
    components/dali/dali.h
    components/dali/lw14.h
    src/linuxi2c.h
)
