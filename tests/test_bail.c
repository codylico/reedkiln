/* SPDX-License-Identifier: Unlicense */
#include "../reedkiln.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int test_setup_fail(void*);
int test_zeta(void*);

void* setup_bad(void*);
void teardown_bad(void*);

struct reedkiln_box const box_bad = { setup_bad, teardown_bad };

struct reedkiln_entry tests[] = {
  { "setup_fail", test_setup_fail, Reedkiln_TODO, &box_bad },
  { "zeta", test_zeta },
  { NULL, NULL }
};


/* test failed setup and bail out support */
void* setup_bad(void* p) {
  unsigned int* num = NULL;
  unsigned int put_str = reedkiln_rand();
  reedkiln_bail_out((put_str&1) ? "setup was bad." : NULL);
  return num;
}
void teardown_bad(void* box) {
  free(box);
  return;
}
int test_setup_fail(void* box) {
  unsigned int v = reedkiln_rand();
  fprintf(stderr, "# %u\n", v);
  if (v & 2)
    reedkiln_fail();
  return Reedkiln_OK;
}

/* last test to run (not required) */
int test_zeta(void* p) {
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln_main(tests, argc, argv, NULL);
}
