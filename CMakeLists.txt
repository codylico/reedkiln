cmake_minimum_required(VERSION 3.0)
project(Reedkiln_test C)

# adapted from:
# https://cliutils.gitlab.io/modern-cmake/chapters/testing.html
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

add_library(reedkiln STATIC "reedkiln.c" "reedkiln.h")

add_subdirectory(tests)

