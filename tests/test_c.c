/* SPDX-License-Identifier: Unlicense */
#include "../reedkiln.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int test_ignore(void*);
int test_memrand(void*);
int test_rand(void*);
int test_setup(void*);
int test_skip(void*);
int test_todo(void*);
int test_todo_ignore(void*);
int test_explicit_false(void*);
int test_explicit_true(void*);
int test_zeta(void*);

void* setup_one(void*);
void teardown_one(void*);

struct reedkiln_box const box_one = { setup_one, teardown_one };

struct reedkiln_entry tests[] = {
  { "memrand", test_memrand },
  { "rand", test_rand },
  { "setup", test_setup, Reedkiln_TODO, &box_one },
  { "skip", test_skip, Reedkiln_SKIP },
  { "todo", test_todo, Reedkiln_TODO },
  { "ignore", test_ignore, 0, &box_one },
  { "todo_ignore", test_todo_ignore, Reedkiln_TODO },
  { "explicit_false", test_explicit_false, Reedkiln_TODO },
  { "explicit_true", test_explicit_true },
  { "zeta", test_zeta },
  { NULL, NULL }
};


/* test ingore result */
int test_ignore(void* p) {
  return Reedkiln_IGNORE;
}

/* test resilience of random number generator */
int test_memrand(void* p) {
  char v[11];
  char w[11];
  reedkiln_memrand(v, 11);
  reedkiln_memrand(w, 11);
  if (memcmp(v, w, 11) == 0)
    reedkiln_fail();
  return Reedkiln_OK;
}
/* test resilience of random number generator */
int test_rand(void* p) {
  unsigned int v = reedkiln_rand();
  unsigned int w = reedkiln_rand();
  if (v == w)
    reedkiln_fail();
  fprintf(stderr, "# %u != %u\n", v, w);
  return Reedkiln_OK;
}
/* test setup and teardown */
void* setup_one(void* p) {
  unsigned int* num = calloc(1,sizeof(unsigned int));
  if (num != NULL) {
    *num = reedkiln_rand();
    fprintf(stderr, "# %u\n", *num);
  }
  return num;
}
void teardown_one(void* box) {
  free(box);
  return;
}
int test_setup(void* box) {
  unsigned int v = *(unsigned int*)box;
  fprintf(stderr, "# %u\n", v);
  if (v & 2)
    reedkiln_fail();
  return Reedkiln_OK;
}


/* test proper test skipping */
int test_skip(void* p) {
  reedkiln_fail();
  /* [[unreachable]] */return Reedkiln_NOT_OK;
}

/* test for expectation of failure */
int test_todo(void* p) {
  return Reedkiln_NOT_OK;
}

/* test for expectation of failure that gets ignored */
int test_todo_ignore(void* p) {
  return Reedkiln_IGNORE;
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
