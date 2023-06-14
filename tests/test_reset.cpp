#include "../log.h"
#include "../reedkiln.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

int test_in_range(void*);
int test_out_range(void*);
int test_zeta(void*);
static bool found_zeta(int argc, char**argv);

struct reedkiln_entry tests[] = {
  { "in_range", test_in_range, 0, reedkiln::expect_box<void,
        reedkiln::cxx_accept<std::out_of_range>
      >::ptr },
  { "out_range", test_out_range },
  { "zeta", test_zeta },
  { NULL, NULL }
};

/* add the out_of_range check */
int test_in_range(void* p) {
  throw std::out_of_range("range check");
  return Reedkiln_NOT_OK;
}

/* test that the out_of_range check is gone */
int test_out_range(void* p) {
  throw std::out_of_range("range check");
  return Reedkiln_OK;
}

/* last test to run */
int test_zeta(void* p) {
  reedkiln::cxx_log() << "Test 'out_range' throws an exception "
    "that it does not want caught, so this test should be "
    "unreachable!" << std::flush;
  return Reedkiln_NOT_OK;
}

static
bool found_zeta(int argc, char**argv) {
  int argi;
  bool skip = false;
  for (argi = 1; argi < argc; ++argi) {
    if (skip)
      skip = false;
    else if (std::strcmp(argv[argi],"-s") == 0)
      skip = true;
    else if (argv[argi][0] == '-')
      continue;
    else
    {
      std::size_t const len = std::strlen(argv[argi]);
      return (len <= 4 && std::strncmp(argv[argi],"zeta",len) == 0);
    }
  }
  return true;
}

int main(int argc, char **argv) {
  try {
    return reedkiln::cxx_main(tests, argc, argv, 0);
  } catch (std::out_of_range const&) {
    bool const want_zeta = found_zeta(argc, argv);
    /* add placeholder TAP output */
    std::cout << "ok 2 - out_range\n"
    << "  ---\n"
    << "  message: \"Exception passed through successfully.\"\n"
    << "  ...\n"
    << "ok 3 - zeta" << (want_zeta ? "" : " # SKIP by request")
    << std::endl;
    return EXIT_SUCCESS;
  }
}
