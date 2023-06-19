/* SPDX-License-Identifier: Unlicense */
#include "../reedkiln.h"
#include "../log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <float.h>


int test_assert(void*);
int test_assert_fail(void*);
int test_explicit_false(void*);
int test_explicit_true(void*);
int test_message(void*);
int test_message_percent(void*);
int test_message_s(void*);
int test_message_s_precision(void*);
int test_message_ls(void*);
int test_message_ls_precision(void*);
int test_message_x(void*);
int test_message_x_precision(void*);
int test_message_p(void*);
int test_message_a(void*);
int test_message_a_limits(void*);
int test_message_a_rand(void*);
int test_message_i(void*);
int test_message_i_negative(void*);
int test_message_i_precision(void*);
int test_message_i_size(void*);
int test_zeta(void*);

struct reedkiln_entry tests[] = {
  { "assert", test_assert },
  { "assert_fail", test_assert_fail, Reedkiln_TODO },
  { "explicit_false", test_explicit_false, Reedkiln_TODO },
  { "explicit_true", test_explicit_true },
  { "message", test_message },
  { "message_percent", test_message_percent },
  { "message_s", test_message_s },
  { "message_s/precision", test_message_s_precision },
  { "message_ls", test_message_ls },
  { "message_ls/precision", test_message_ls_precision },
  { "message_x", test_message_x },
  { "message_x/precision", test_message_x_precision },
  { "message_p", test_message_p },
  { "message_a", test_message_a },
  { "message_a/limits", test_message_a_limits },
  { "message_a/rand", test_message_a_rand },
  { "message_i", test_message_i },
  { "message_i/precision", test_message_i_precision },
  { "message_i/negative", test_message_i_negative },
  { "message_i/size", test_message_i_size },
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
/* test message output */
int test_message(void* p) {
  char const str[] = "Hello, world!";
  reedkiln_log_write(str, sizeof(str)-1);
  return Reedkiln_OK;
}
/* test message output : %% */
int test_message_percent(void* p) {
  size_t bytes = reedkiln_log_printf("100%%");
  reedkiln_assert(bytes == 5);
  return Reedkiln_OK;
}
/* test message output : %s */
int test_message_s(void* p) {
  char const str[] = "Hello, world!";
  size_t bytes = reedkiln_log_printf("Quotation: \"%s\"", str);
  reedkiln_assert(bytes == 13+14);
  return Reedkiln_OK;
}
/* test message output : %*s */
int test_message_s_precision(void* p) {
  char const str[] = "abcdefghijklm";
  unsigned int const num = 1+reedkiln_rand()%12;
  size_t bytes = reedkiln_log_printf("Quotation: \"%.*s\" %.5s",
    num, str, str);
  reedkiln_assert(bytes == 15+num+5);
  return Reedkiln_OK;
}
/* test message output : %ls */
int test_message_ls(void* p) {
  wchar_t const str[] = L"Hello, world!";
  wchar_t const str2[] = { 0x101, 0x250C, 0 };
  size_t bytes = reedkiln_log_printf("Wide %ls: ", str);
  reedkiln_log_printf("%ls", str2);
  reedkiln_assert(bytes == 8+13);
  return Reedkiln_OK;
}
/* test message output : %.*ls */
int test_message_ls_precision(void* p) {
  wchar_t const str[] = L"abcdefghijklm";
  unsigned int const num = 1+reedkiln_rand()%12;
  size_t bytes = reedkiln_log_printf("Wide %.*ls", num, str);
  reedkiln_assert(bytes == 6+num);
  return Reedkiln_OK;
}
/* test message output : %x */
int test_message_x(void* p) {
  unsigned int num = reedkiln_rand();
  size_t bytes = reedkiln_log_printf("Number %x.", num);
  size_t expected = 0;
  if (num == 0)
    expected = 1;
  else while (num > 0) {
    num >>= 4;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+9);
  return Reedkiln_OK;
}
/* test message output : %p */
int test_message_p(void* p) {
  size_t bytes = reedkiln_log_printf("Pointer %p != %p.", &p, NULL);
  reedkiln_assert(bytes > 10);
  return Reedkiln_OK;
}
/* test message output : %a */
int test_message_a(void* p) {
  size_t bytes = reedkiln_log_printf("Float %a %a %a %a.",
    1.0, 0.0, -1.0, HUGE_VAL);
  reedkiln_assert(bytes >= 10+7);
  return Reedkiln_OK;
}
/* test message output : %a at the limits */
int test_message_a_limits(void* p) {
  size_t bytes = reedkiln_log_printf("Float %a %a %a %a.",
    DBL_MAX, DBL_MIN, DBL_EPSILON, FLT_EPSILON);
  reedkiln_assert(bytes >= 10+7);
  return Reedkiln_OK;
}
/* test message output : %a with random inputs */
int test_message_a_rand(void* p) {
  unsigned int num = reedkiln_rand();
  double v = ((num&1) ? -1.0 : 1.0) * (num * 256.0 / UINT_MAX);
  double w = ((num&1) ? -1.0 : 1.0) * (num / 256.0 / UINT_MAX);
  size_t bytes = reedkiln_log_printf("%a * %a = %a;", v, w, v*w);
  reedkiln_assert(bytes > 10);
  return Reedkiln_OK;
}

/* test message output : %i */
int test_message_i(void* p) {
  unsigned int num = reedkiln_rand()%INT_MAX;
  unsigned int const old_num = num;
  size_t bytes = reedkiln_log_printf("Number %i.", old_num);
  int expected = 0;
  if (num == 0)
    expected = 1;
  else while (num > 0) {
    num /= 10;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+9);
  return Reedkiln_OK;
}
/* test message output : %zx */
int test_message_x_precision(void* p) {
  size_t const positive = ((~(size_t)0)>>1);
  size_t num = (reedkiln_rand()*(size_t)12345) & positive;
  size_t const old_num = num;
  size_t bytes = reedkiln_log_printf("Number %0zX.", old_num);
  int expected = 0;
  if (num == 0)
    expected = 1;
  else while (num > 0) {
    num >>= 4;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+9);
  return Reedkiln_OK;
}
/* test message output : %*.*zi */
int test_message_i_precision(void* p) {
  size_t const positive = ((~(size_t)0)>>1);
  size_t num = (reedkiln_rand()*(size_t)12345) & positive;
  size_t const old_num = num;
  size_t bytes = reedkiln_log_printf("Number %*.*zi.", 1, 1, old_num);
  size_t expected = 0;
  if (num == 0)
    expected = 1;
  else while (num > 0) {
    num /= 10;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+9);
  return Reedkiln_OK;
}

/* test message output : %zi */
int test_message_i_size(void* p) {
  size_t const positive = ((~(size_t)0)>>1);
  size_t num = (reedkiln_rand()*(size_t)12345) & positive;
  size_t const old_num = num;
  size_t bytes = reedkiln_log_printf("Number %zi.", old_num);
  size_t expected = 0;
  if (num == 0)
    expected = 1;
  else while (num > 0) {
    num /= 10;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+9);
  return Reedkiln_OK;
}
/* test message output : %i negative */
int test_message_i_negative(void* p) {
  int const old_num = -(int)(reedkiln_rand()%INT_MAX);
  size_t bytes = reedkiln_log_printf("Number %i.", old_num);
  size_t expected = 0;
  int num = -old_num;
  if (num == 0)
    expected = 0;
  else while (num > 0) {
    num /= 10;
    expected += 1;
  }
  reedkiln_assert(bytes == expected+10);
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
