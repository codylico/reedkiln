cmake_minimum_required(VERSION 3.1)
project(Reedkiln_test C)

# adapted from:
# https://cliutils.gitlab.io/modern-cmake/chapters/testing.html
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

add_library(reedkiln STATIC "reedkiln.c" "reedkiln.h")
target_compile_definitions(reedkiln
  INTERFACE "$<$<COMPILE_FEATURES:cxx_noexcept>:Reedkiln_UseNoexcept=1>"
  "$<$<COMPILE_FEATURES:cxx_variadic_templates>:Reedkiln_UseExpect=1>")

add_subdirectory(tests)
