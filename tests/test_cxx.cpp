// SPDX-License-Identifier: Unlicense
#include "../reedkiln.h"
#include <string>
#include <iostream>
#include <cstring>

class free_box {
private:
  int *data;
  int n;
public:
  free_box(int count)
    : data(count > 0 ? new int[count] : 0), n(count)
  {
  }
  ~free_box() {
    if (data)
      delete[] data;
  }
  free_box(free_box const& b)
    : data(b.n > 0 ? new int[b.n] : 0), n(b.n)
  {
    std::memcpy(data, b.data, n*sizeof(int));
  }
  free_box& operator=(free_box const& b) {
    int* new_data;
    int new_count;
    if (b.n > 0) {
      new_count = b.n;
      new_data = new int[b.n];
      std::memcpy(new_data, b.data, n*sizeof(int));
    } else {
      new_data = 0;
      new_count = 0;
    }
    if (data)
      delete[] data;
    data = new_data;
    n = new_count;
    return *this;
  }
};
class seven_checker {
private:
  int x;
public:
  seven_checker(int a) : x(a) {}
  operator bool() const {
    return x >= 7;
  }
};

int test_cxx_raii(void*);
int test_cxx_assert(void*);
int test_cxx_setup(void*);
int test_cxx_setupfail(void*);
int test_memrand(void*);
int test_rand(void*);
int test_skip(void*);
int test_todo(void*);
int test_cxx_explicit_false(void*);
int test_cxx_explicit_true(void*);
int test_zeta(void*);

struct reedkiln_entry tests[] = {
  { "cxx/raii", test_cxx_raii, Reedkiln_TODO },
  { "cxx/assert", test_cxx_assert, Reedkiln_TODO,
    reedkiln::cxx_box<std::string>::ptr },
  { "cxx/setup", test_cxx_setup, 0,
    reedkiln::cxx_box<std::string>::ptr },
  { "cxx/setupfail", test_cxx_setupfail, Reedkiln_TODO,
    reedkiln::cxx_box<std::string>::ptr },
  { "memrand", test_memrand },
  { "rand", test_rand },
  { "skip", test_skip, Reedkiln_SKIP },
  { "todo", test_todo, Reedkiln_TODO },
  { "cxx/explicit_true", test_cxx_explicit_true },
  { "cxx/explicit_false", test_cxx_explicit_false, Reedkiln_TODO },
  { "zeta", test_zeta },
  { NULL, NULL }
};

/* test resilience of random number generator */
int test_cxx_raii(void* p) {
  free_box box(15);
  reedkiln_fail();
  return Reedkiln_OK;
}
/* test cleanup with assert */
int test_cxx_assert(void* p) {
  std::string &str = *static_cast<std::string*>(p);
  str.push_back('h');
  str.append("ello,");
  str += " world!";
  reedkiln_assert(str.size() != 13);
  return Reedkiln_OK;
}
/* test auto-generated box */
int test_cxx_setup(void* p) {
  std::string &str = *static_cast<std::string*>(p);
  str.push_back('h');
  str.append("ello,");
  str += " world!";
  std::cerr << "# " << str << std::endl;
  return Reedkiln_OK;
}
/* test cleanup of auto-generated box */
int test_cxx_setupfail(void* p) {
  std::string &str = *static_cast<std::string*>(p);
  str.append("oops.");
  reedkiln_fail();
  return Reedkiln_OK;
}

/* test resilience of random number generator */
int test_memrand(void* p) {
  char v[11];
  char w[11];
  reedkiln_memrand(v, 11);
  reedkiln_memrand(w, 11);
  if (std::memcmp(v, w, 11) == 0)
    reedkiln_fail();
  return Reedkiln_OK;
}
/* test resilience of random number generator */
int test_rand(void* p) {
  unsigned int v = reedkiln_rand();
  unsigned int w = reedkiln_rand();
  if (v == w)
    reedkiln_fail();
  return Reedkiln_OK;
}

/* test proper test skipping */
int test_skip(void* p) {
  reedkiln_fail();
  /* [[unreachable]] */return Reedkiln_NOT_OK;
}

/* test proper assert bool conversion */
int test_cxx_explicit_false(void* p) {
  seven_checker x(6 - reedkiln_rand()%16);
  reedkiln_assert(x);
  return Reedkiln_OK;
}
int test_cxx_explicit_true(void* p) {
  seven_checker x(reedkiln_rand()%16 + 7);
  reedkiln_assert(x);
  reedkiln_assert(&x);
  reedkiln_assert(std::cout);
  return Reedkiln_OK;
}

/* test for expectation of failure */
int test_todo(void* p) {
  return Reedkiln_NOT_OK;
}

/* last test to run */
int test_zeta(void* p) {
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln::cxx_main(tests, argc, argv, 0);
}
