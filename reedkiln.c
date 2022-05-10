/* SPDX-License-Identifier: Unlicense */
/**
 * @file reedkiln.c
 * @brief Short test implementation.
 */
#include "reedkiln.h"

#if !defined(Reedkiln_Atomic)
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) \
       && (!defined(__STDC_NO_ATOMICS__))
#    define Reedkiln_Atomic
#  endif /*__STDC_NO_ATOMICS__, __STDC_VERSION__*/
#endif
#if !defined(Reedkiln_Threads)
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) \
       && (!defined(__STDC_NO_THREADS__))
#    define Reedkiln_Threads
#  endif /*__STDC_NO_THREADS__, __STDC_VERSION__*/
#endif

#if defined(Reedkiln_Atomic)
#  include <stdatomic.h>
#  define Reedkiln_Atomic_tag _Atomic
#  define Reedkiln_Atomic_Xchg(c,v) \
      atomic_exchange_explicit((c),(v),memory_order_acq_rel)
#  define Reedkiln_Atomic_Put(c,v) \
      atomic_store_explicit((c),(v),memory_order_release)
#  define Reedkiln_Atomic_Get(c) atomic_load_explicit((c),memory_order_acquire)
#endif /*Reedkiln_Atomic*/
#if defined(Reedkiln_Threads)
#  include <threads.h>
#  define Reedkiln_Thread_local _Thread_local
#else
#  define Reedkiln_Thread_local
#endif /*Reedkiln_Threads*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

static size_t reedkiln_entry_count(struct reedkiln_entry const* t);
static char const* reedkiln_entry_directive(struct reedkiln_entry const* t);
static void reedkiln_srand(unsigned int s);
static unsigned int reedkiln_rand_step(void);
static int reedkiln_prefix_match(char const* name, char const* prefix);
static int reedkiln_passthrough(reedkiln_cb cb, void* ptr);
static void reedkiln_failfast(void);
static int reedkiln_run_test(reedkiln_cb cb, void* p);
static unsigned int reedkiln_default_seed(void);

static struct reedkiln_vtable reedkiln_vtable_c = {
  &reedkiln_passthrough,
  &reedkiln_failfast
};

struct reedkiln_jmp {
  unsigned int active;
  jmp_buf buf;
};

/* BEGIN failure path */
#if defined(Reedkiln_Atomic)
static Reedkiln_Atomic_tag unsigned int reedkiln_next_status;
#else
static unsigned int reedkiln_next_status;
#endif /*Reedkiln_Atomic*/

static Reedkiln_Thread_local struct reedkiln_jmp reedkiln_next_jmp = {0};

void reedkiln_fail(void) {
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_Put(&reedkiln_next_status, Reedkiln_NOT_OK);
#else
  reedkiln_next_status = Reedkiln_NOT_OK;
#endif /*Reedkiln_Atomic*/
  (*reedkiln_vtable_c.fail_cb)();
  abort()/* in case the above doesn't work */;
}

int reedkiln_run_test(reedkiln_cb cb, void* p) {
  int res;
  Reedkiln_Atomic_Put(&reedkiln_next_status, Reedkiln_OK);
  res = (*reedkiln_vtable_c.catch_cb)(cb, p);
  reedkiln_next_jmp.active = 0u;
  return res == Reedkiln_OK
    ? Reedkiln_Atomic_Get(&reedkiln_next_status)
    : res;
}
/* END   failure path */

/* BEGIN random stuff */
#if defined(Reedkiln_Atomic)
static Reedkiln_Atomic_tag unsigned int reedkiln_rand_seed = 0u;
#else
static unsigned int reedkiln_rand_seed = 0u;
#endif /*Reedkiln_Atomic*/


unsigned int reedkiln_rand(void) {
  return reedkiln_rand_step();
}

unsigned int reedkiln_rand_step(void) {
  static unsigned int const mul = 48271u;
  static unsigned int const add = 9u;
#if defined(Reedkiln_Atomic)
  unsigned int src = Reedkiln_Atomic_Get(&reedkiln_rand_seed);
  unsigned int in, out;
  do {
    in = src;
    out = in*mul+add;
  } while ((src = Reedkiln_Atomic_Xchg(&reedkiln_rand_seed,out)) != in);
#else
  unsigned int out = reedkiln_rand_seed*mul+add;
  reedkiln_rand_seed = out;
#endif /*Reedkiln_Atomic*/
  return out;
}

void reedkiln_srand(unsigned int s) {
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_Put(&reedkiln_rand_seed, s);
#else
  reedkiln_rand_seed = s;
#endif /*Reedkiln_Atomic*/
}

unsigned int reedkiln_default_seed(void) {
  unsigned long int const src = (unsigned long int)time(NULL);
  return (unsigned int)(src*0x87e5c341);
}

void reedkiln_memrand(void* b, reedkiln_size sz) {
  size_t xsz = sz - (sz%sizeof(unsigned int));
  size_t xi;
  unsigned char* const p = (unsigned char*)b;
  for (xi = 0u; xi < xsz; xi += sizeof(unsigned int)) {
    unsigned int const next = reedkiln_rand();
    memcpy(p+xi, &next, sizeof(unsigned int));
  }
  if (xsz < sz) {
    size_t const rem = sz - xsz;
    unsigned int const next = reedkiln_rand();
    memcpy(p+xi, &next, rem);
  }
  return;
}
/* END   random stuff */

/* BEGIN error jump */
int reedkiln_passthrough(reedkiln_cb cb, void* ptr) {
  if (setjmp(reedkiln_next_jmp.buf) != 0) {
    reedkiln_next_jmp.active = 0u;
    return Reedkiln_NOT_OK;
  }
  reedkiln_next_jmp.active = 1u;
  /* */{
    int const res = (*cb)(ptr);
    reedkiln_next_jmp.active = 0u;
    return res;
  }
}

void reedkiln_failfast(void) {
  if (reedkiln_next_jmp.active) {
    longjmp(reedkiln_next_jmp.buf, 1);
  } else {
    reedkiln_next_status = Reedkiln_NOT_OK;
    abort();
  }
}
/* END   error jump */


size_t reedkiln_entry_count(struct reedkiln_entry const* t) {
  size_t n;
  for (n = 0u; t[n].cb != NULL; ++n) {
    /* pass */;
  }
  return n;
}
char const* reedkiln_entry_directive(struct reedkiln_entry const* t) {
  if (t->flags & Reedkiln_SKIP) {
    return " # SKIP";
  } else if (t->flags & Reedkiln_TODO) {
    return " # TODO";
  } else return "";
}

int reedkiln_prefix_match(char const* name, char const* prefix) {
  char const* name_p;
  char const* prefix_p;
  for (name_p = name, prefix_p = prefix;
      *name_p != '\0' && *prefix_p != '\0'; ++name_p, ++prefix_p)
  {
    if (*name_p != *prefix_p)
      return 0;
  }
  return *prefix_p == '\0';
}

void reedkiln_set_vtable(struct reedkiln_vtable const* vt) {
  reedkiln_vtable_c = *vt;
  return;
}

int reedkiln_main
    (struct reedkiln_entry const* t, int argc, char **argv, void* p)
{
  size_t const test_count = reedkiln_entry_count(t);
  size_t test_i;
  int total_res = EXIT_SUCCESS;
  char const* testname_prefix = "";
  unsigned int rand_seed = reedkiln_default_seed();
  /* inspect args */{
    int argi;
    int help_tf = 0;
    for (argi = 1; argi < argc; ++argi) {
      if (argv[argi][0] == '-') {
        if (strcmp(argv[argi], "-?") == 0
        ||  strcmp(argv[argi], "-h") == 0)
        {
          help_tf = 1;
          break;
        } else if (strcmp(argv[argi], "-l") == 0) {
          help_tf = 2;
        } else if (strcmp(argv[argi], "-s") == 0) {
          if (++argi >= argc) {
            fputs("option \"-s\" requires a number\n", stderr);
            help_tf = 1;
          } else {
            rand_seed = (unsigned int)strtoul(argv[argi], NULL, 0);
          }
        } else {
          fprintf(stderr,"unknown option \"%s\"\n", argv[argi]);
          help_tf = 1;
          break;
        }
      } else {
        testname_prefix = argv[argi];
      }
    }
    if (help_tf) {
      if (help_tf == 2) {
        size_t i;
        for (i = 0u; i < test_count; ++i) {
          fprintf(stderr, "%s\n", t[i].name);
        }
      } else {
        fputs("usage: %s [option [option ...]] [(prefix)]\n\n"
          "options:\n"
          "  -?, -h      print a help message\n"
          "  -l          list all test names\n"
          "  -s (seed)   set the random seed\n\n"
          "parameters:\n"
          "  (prefix)    run tests whose names start with this prefix\n",
          stderr);
      }
      return EXIT_FAILURE;
    }
  }
  fprintf(stdout,"1..%lu\n", (unsigned long int)test_count);
  fprintf(stdout,"# random_seed: %#x\n", rand_seed);
  for (test_i = 0; test_i < test_count; ++test_i) {
    struct reedkiln_entry const* test = t+test_i;
    char const* result_text;
    char const* direct_text = reedkiln_entry_directive(test);
    int skip_tf = ((test->flags & Reedkiln_SKIP)!= 0)
        || (!reedkiln_prefix_match(test->name, testname_prefix));
    int res = 0;
    if (!skip_tf) {
      struct reedkiln_box const* box = test->box;
      void* box_item = NULL;
      int box_called = 0;
      reedkiln_srand(rand_seed);
      if (box != NULL && box->setup != NULL) {
        box_item = (*box->setup)(p);
        box_called = 1;
      }
      res = reedkiln_run_test(test->cb, box_called ? box_item : p);
      if (box_called && box->teardown != NULL) {
        (*box->teardown)(box_item);
      }
    }
    switch (res) {
    case 0:
      result_text = "ok"; break;
    default:
      if (!(test->flags & Reedkiln_TODO))
        total_res = EXIT_FAILURE;
      result_text = "not ok";break;
    }
    fprintf(stdout, "%s %lu - %s%s\n",
      result_text, ((unsigned long int)(test_i+1)), t[test_i].name,
      direct_text);
  }
  return total_res;
}
