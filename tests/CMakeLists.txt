cmake_minimum_required(VERSION 3.0)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(Reedkiln_BUILD_TESTING "Enable tests for Reedkiln" ON)
else (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(Reedkiln_BUILD_TESTING "Enable tests for Reedkiln" OFF)
endif (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

if (Reedkiln_BUILD_TESTING AND BUILD_TESTING)
  option(Reedkiln_TEST_CXX "Enable C++ test" "${Reedkiln_ADD_CXX}")
  if (Reedkiln_ADD_CXX AND Reedkiln_TEST_CXX)
    enable_language(CXX)
    add_executable(reedkiln_test_cxx "test_cxx.cpp")
    target_link_libraries(reedkiln_test_cxx
      PRIVATE reedkiln)
    add_test(NAME "reedkiln::cxx" COMMAND reedkiln_test_cxx)

    add_executable(reedkiln_test_raii_bail "test_raii_bail.cpp")
    target_link_libraries(reedkiln_test_raii_bail
      PRIVATE reedkiln)
    add_test(NAME "reedkiln::raii_bail" COMMAND reedkiln_test_raii_bail)
    set_tests_properties("reedkiln::raii_bail"
      PROPERTIES WILL_FAIL TRUE)

    add_executable(reedkiln_test_expect "test_expect.cpp")
    target_link_libraries(reedkiln_test_expect
      PRIVATE reedkiln)
    target_compile_features(reedkiln_test_expect
      PRIVATE cxx_variadic_templates cxx_static_assert cxx_constexpr
      )
    add_test(NAME "reedkiln::expect" COMMAND reedkiln_test_expect)

    add_executable(reedkiln_test_reset "test_reset.cpp")
    target_link_libraries(reedkiln_test_reset
      PRIVATE reedkiln)
    target_compile_features(reedkiln_test_reset
      PRIVATE cxx_variadic_templates cxx_static_assert cxx_constexpr
      )
    add_test(NAME "reedkiln::reset" COMMAND reedkiln_test_reset)
  endif (Reedkiln_ADD_CXX AND Reedkiln_TEST_CXX)

  add_executable(reedkiln_test_c "test_c.c")
  target_link_libraries(reedkiln_test_c
    PRIVATE reedkiln)
  # adapted from:
  # https://cliutils.gitlab.io/modern-cmake/chapters/testing.html
  add_test(NAME "reedkiln::c" COMMAND reedkiln_test_c)

  add_executable(reedkiln_test_bail "test_bail.c")
  target_link_libraries(reedkiln_test_bail
    PRIVATE reedkiln)
  add_test(NAME "reedkiln::bail" COMMAND reedkiln_test_bail)
  set_tests_properties("reedkiln::bail"
    PROPERTIES WILL_FAIL TRUE)

  add_executable(reedkiln_test_assert "test_assert.c")
  target_link_libraries(reedkiln_test_assert
    PRIVATE reedkiln)
  add_test(NAME "reedkiln::assert" COMMAND reedkiln_test_assert)
endif (Reedkiln_BUILD_TESTING AND BUILD_TESTING)

