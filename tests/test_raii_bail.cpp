// SPDX-License-Identifier: Unlicense
#include "../reedkiln.h"
#include <string>
#include <iostream>


int test_cxx_raii_bail(void*);
int test_zeta(void*);

class break_easy {
private:
  std::string s;
public:
  break_easy() {
    s += "some text,";
    s += "some more text, ";
    /* oops. not supposed to work yet! */
    reedkiln_bail_out("break_easy broke.");
  };
  std::string get_string() {
    return s;
  }
};

struct reedkiln_entry tests[] = {
  { "cxx/raii_bail", test_cxx_raii_bail, 0,
    &reedkiln::cxx_box<break_easy>::value },
  { "zeta", test_zeta },
  { NULL, NULL }
};

/* test resilience of random number generator */
int test_cxx_raii_bail(void* p) {
  break_easy& data = *static_cast<break_easy*>(p);
  std::cerr << "# " << data.get_string() << std::endl;
  return Reedkiln_OK;
}

/* last test to run */
int test_zeta(void* p) {
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln::cxx_main(tests, argc, argv, 0);
}
