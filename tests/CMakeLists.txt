cmake_minimum_required(VERSION 3.0)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(Reedkiln_BUILD_TESTING "Enable tests for Reedkiln" ON)
else (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(Reedkiln_BUILD_TESTING "Enable tests for Reedkiln" OFF)
endif (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

if (Reedkiln_BUILD_TESTING AND BUILD_TESTING)
  option(Reedkiln_TEST_CXX "Enable C++ test" OFF)
  if (Reedkiln_TEST_CXX)
    enable_language(CXX)
    add_executable(reedkiln_test_cxx "test_cxx.cpp")
    target_link_libraries(reedkiln_test_cxx
      PRIVATE reedkiln)
    add_test(NAME "reedkiln::cxx" COMMAND reedkiln_test_cxx)
  endif (Reedkiln_TEST_CXX)
  
  add_executable(reedkiln_test_c "test_c.c")
  target_link_libraries(reedkiln_test_c
    PRIVATE reedkiln)
  # adapted from:
  # https://cliutils.gitlab.io/modern-cmake/chapters/testing.html
  add_test(NAME "reedkiln::c" COMMAND reedkiln_test_c)
endif (Reedkiln_BUILD_TESTING AND BUILD_TESTING)

