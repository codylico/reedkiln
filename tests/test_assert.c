/* SPDX-License-Identifier: Unlicense */
#include "../reedkiln.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int test_assert(void*);
int test_assert_fail(void*);
int test_explicit_false(void*);
int test_explicit_true(void*);
int test_zeta(void*);

struct reedkiln_entry tests[] = {
  { "assert", test_assert },
  { "assert_fail", test_assert_fail, Reedkiln_TODO },
  { "explicit_false", test_explicit_false, Reedkiln_TODO },
  { "explicit_true", test_explicit_true },
  { "zeta", test_zeta },
  { NULL, NULL }
};

/* test assertion processing */
int test_assert(void* p) {
  unsigned int v = reedkiln_rand();
  unsigned int w = reedkiln_rand();
  reedkiln_assert(v != w);
  return Reedkiln_OK;
}
/* test assertion handling */
int test_assert_fail(void* p) {
  unsigned int v = reedkiln_rand();
  unsigned int w = reedkiln_rand();
  reedkiln_assert(v == w);
  return Reedkiln_OK;
}

/* test for assert conversion to Boolean false */
int test_explicit_false(void* p) {
  reedkiln_assert(NULL);
  return Reedkiln_OK;
}

/* test for assert conversion to Boolean true */
int test_explicit_true(void* p) {
  reedkiln_assert(&tests);
  reedkiln_assert(&test_explicit_true);
  reedkiln_assert(3.14159);
  reedkiln_assert("a string");
  reedkiln_assert('5');
  reedkiln_assert(0.5);
  return Reedkiln_OK;
}

/* last test to run */
int test_zeta(void* p) {
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests, argc, argv, NULL);
}
