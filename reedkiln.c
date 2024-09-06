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
#elif (defined _MSC_VER)
#  include <intrin.h>
#  include <limits.h>
#  define Reedkiln_Atomic_tag
#  if (ULONG_MAX == UINT_MAX)
#    define Reedkiln_Atomic_Xchg(c,v) \
        _InterlockedExchange((c), (v))
#  else
#    define Reedkiln_Atomic_Xchg(c,v) \
        _InterlockedExchange16((c), (v))
#  endif /*UINT_MAX*/
static void Reedkiln_Atomic_Put(unsigned int volatile* c, unsigned int v) {
  *c = v;
}
static unsigned int Reedkiln_Atomic_Get(unsigned int const volatile* c) {
  return *c;
}
#  define Reedkiln_Atomic
#endif /*Reedkiln_Atomic*/
#if defined(Reedkiln_Threads)
#  include <threads.h>
#  define Reedkiln_Thread_local _Thread_local
#elif (defined _MSC_VER)
#  define Reedkiln_Thread_local __declspec(thread)
#else
#  define Reedkiln_Thread_local
#endif /*Reedkiln_Threads*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <stdarg.h>
#include <errno.h>

struct reedkiln_logbuf;

#if defined(ULLONG_MAX)
typedef unsigned long long reedkiln_intmax;
#else
typedef unsigned long reedkiln_intmax;
#endif /*ULLONG_MAX*/

static size_t reedkiln_entry_count(struct reedkiln_entry const* t);
static char const* reedkiln_entry_directive(struct reedkiln_entry const* t);
static void reedkiln_srand(unsigned int s);
static unsigned int reedkiln_rand_step(void);
static int reedkiln_prefix_match(char const* name, char const* prefix);
static int reedkiln_passthrough(reedkiln_cb cb, void* ptr);
static void reedkiln_failfast(void);
static int reedkiln_run_test(reedkiln_cb cb, void* p);
static int reedkiln_setup_redirect(void* d);
static int reedkiln_run_setup(reedkiln_setup_cb cb, void* p, void** out);
static unsigned int reedkiln_default_seed(void);
static void reedkiln_print_bail(char const* reason);
static unsigned int reedkiln_log_swap(void);
static void reedkiln_log_reset(void);
static unsigned int reedkiln_log_nextpos
  (struct reedkiln_logbuf* ptr, reedkiln_size n);
static void reedkiln_log_escape(unsigned char const* data, size_t n, FILE* f);
static int reedkiln_log_setline(char const* name, unsigned long int lineno);
static int reedkiln_isslash(int ch);
static int reedkiln_log_vsnprintf
  (unsigned char* output, size_t sz, char const* format, va_list ap);
static int reedkiln_log_itox
  (unsigned char* output, reedkiln_intmax value);
static int reedkiln_log_itoa
  (unsigned char* output, reedkiln_intmax value);
static int reedkiln_log_dtox
  (unsigned char* output, long double value);
static unsigned int reedkiln_log_count_str
  (unsigned char* output, size_t sz, unsigned count, char const* str);
static unsigned int reedkiln_log_count_ncat
  ( unsigned char* output, size_t sz, unsigned count,
    unsigned precision, char const* str);
static unsigned int reedkiln_log_count_strn
  ( unsigned char* output, size_t sz, unsigned count,
    unsigned int slen, char const* str);
static unsigned int reedkiln_log_count_revn
  ( unsigned char* output, size_t sz, unsigned count,
    int slen, unsigned char const* str);

struct reedkiln_log_attr {
  unsigned char value;
  unsigned char width;
  unsigned char prec;
};
static
struct reedkiln_log_attr reedkiln_log_adjust_type(char const** p);

enum reedkiln_const {
  /** @note Max length of encoding a wchar_t to UTF-8 */
  Reedkiln_UTF8Max = 6,
  /** @note Max length of formatting a hexadecimal integer */
  Reedkiln_ItoXMax = (sizeof(reedkiln_intmax)*CHAR_BIT+3)/4,
  /** @note Max length of formatting a decimal integer */
  Reedkiln_ItoAMax = (sizeof(reedkiln_intmax)*CHAR_BIT+3)/3,
  /** @note Max length of formatting a hexadecimal integer */
  Reedkiln_DtoXMax = (LDBL_DIG)+3+Reedkiln_ItoAMax,
  /** @note Approximate base-2 minimum long double exponent */
  Reedkiln_DMinExp = (LDBL_MIN_10_EXP - DBL_DIG) * 4,
  /** @note Approximate base-2 maximum long double exponent */
  Reedkiln_DMaxExp = (LDBL_MAX_10_EXP) * 4,
  /** @note Bias for length modifier */
  Reedkiln_Bias = 128u
};

static struct reedkiln_vtable reedkiln_vtable_c = {
  &reedkiln_passthrough,
  &reedkiln_failfast
};

struct reedkiln_jmp {
  unsigned int active;
  jmp_buf buf;
};

struct reedkiln_logbuf {
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_tag unsigned int pos;
#else
  unsigned int pos;
#endif /*Reedkiln_Atomic*/
  unsigned char* data;
};

struct reedkiln_setup_data {
  reedkiln_setup_cb cb;
  void* p;
  void* out;
};

/* BEGIN failure path */
#if defined(Reedkiln_Atomic)
static Reedkiln_Atomic_tag unsigned int reedkiln_next_status = Reedkiln_OK;
static Reedkiln_Atomic_tag unsigned int reedkiln_bail_status = Reedkiln_OK;
#else
static unsigned int reedkiln_next_status = Reedkiln_OK;
static unsigned int reedkiln_bail_status = Reedkiln_OK;
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

void reedkiln_assert_ex
  (int val, char const* text, char const* file, unsigned long int line)
{
  if (!val) {
    fprintf(stdout, "## assert %s:%lu: %s\n", file, line, text);
    reedkiln_fail();
  }
  return;
}

void reedkiln_bail_out(char const* reason) {
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_Put(&reedkiln_next_status, Reedkiln_NOT_OK);
  Reedkiln_Atomic_Put(&reedkiln_bail_status, Reedkiln_NOT_OK);
#else
  reedkiln_next_status = Reedkiln_NOT_OK;
  reedkiln_bail_status = Reedkiln_NOT_OK;
#endif /*Reedkiln_Atomic*/
  reedkiln_print_bail(reason);
  (*reedkiln_vtable_c.fail_cb)();
  /* in case the above doesn't work */{
    abort();
  }
}

void reedkiln_print_bail(char const* reason) {
  fputs("Bail out!", stdout);
  if (reason != NULL) {
    fputc(' ', stdout);
    fputs(reason, stdout);
  }
  fputc('\n', stdout);
  return;
}

int reedkiln_run_test(reedkiln_cb cb, void* p) {
  int res;
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_Put(&reedkiln_next_status, Reedkiln_OK);
#else
  reedkiln_next_status = Reedkiln_OK;
#endif
  res = (*reedkiln_vtable_c.catch_cb)(cb, p);
  reedkiln_next_jmp.active = 0u;
#if defined(Reedkiln_Atomic)
  return res == Reedkiln_OK
    ? Reedkiln_Atomic_Get(&reedkiln_next_status)
    : res;
#else
  return res == Reedkiln_OK
    ? reedkiln_next_status
    : res;
#endif /*Reedkiln_Atomic*/
}

int reedkiln_setup_redirect(void* d) {
  struct reedkiln_setup_data *const data =
    (struct reedkiln_setup_data *)d;
  data->out = (*data->cb)(data->p);
  return Reedkiln_OK;
}

int reedkiln_run_setup(reedkiln_setup_cb cb, void* p, void** out) {
  struct reedkiln_setup_data data;
  int res;
  data.cb = cb;
  data.p = p;
  data.out = NULL;
  res = reedkiln_run_test(reedkiln_setup_redirect, &data);
  *out = data.out;
  return res;
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

/* BEGIN log buffer */
static unsigned char reedkiln_logbuf_default[256] = { 0 };
static struct reedkiln_logbuf reedkiln_log_buffers[2] = {
  { 0, reedkiln_logbuf_default },
  { 0, reedkiln_logbuf_default+sizeof(reedkiln_logbuf_default)/2u }
};
static unsigned int reedkiln_log_size = sizeof(reedkiln_logbuf_default)/2u;
#if defined(Reedkiln_Atomic)
static Reedkiln_Atomic_tag unsigned int reedkiln_log_index = 0u;
#else
static unsigned int reedkiln_log_index = 0u;
#endif /*Reedkiln_Atomic*/

void reedkiln_log_reset(void) {
#if defined(Reedkiln_Atomic)
  unsigned int const swap_index = Reedkiln_Atomic_Get(&reedkiln_log_index)%2u;
#else
  unsigned int const swap_index = reedkiln_log_index%2u;
#endif /*Reedkiln_Atomic*/
  struct reedkiln_logbuf* const ptr = reedkiln_log_buffers+swap_index;
#if defined(Reedkiln_Atomic)
  Reedkiln_Atomic_Put(&ptr->pos, 0);
#else
  ptr->pos = 0;
#endif /*Reedkiln_Atomic*/
  return;
}

unsigned int reedkiln_log_swap(void) {
#if defined(Reedkiln_Atomic)
  unsigned int const src = Reedkiln_Atomic_Get(&reedkiln_log_index);
  Reedkiln_Atomic_Put(&reedkiln_log_index, src+1);
  return src%2u;
#else
  return (reedkiln_log_index++)%2u;
#endif /*Reedkiln_Atomic*/
}

unsigned int reedkiln_log_nextpos
  (struct reedkiln_logbuf* ptr, reedkiln_size n)
{
  unsigned int const top =
     (n > reedkiln_log_size) ? 0 : (unsigned int)(reedkiln_log_size - n);
#if defined(Reedkiln_Atomic)
  unsigned int src = Reedkiln_Atomic_Get(&ptr->pos);
  unsigned int in, out;
  do {
    if (src >= reedkiln_log_size)
      return UINT_MAX;
    in = src;
    out = (in >= top ? reedkiln_log_size : (unsigned)(in + n));
  } while ((src = Reedkiln_Atomic_Xchg(&ptr->pos,out)) != in);
#else
  unsigned int const in = ptr->pos;
  ptr->pos = (in >= top ? reedkiln_log_size : (unsigned)(in + n));
#endif /*Reedkiln_Atomic*/
  return in;
}

reedkiln_size reedkiln_log_write(void const* buffer, reedkiln_size count) {
  unsigned int const n =
    (count >= (UINT_MAX/2)) ? (UINT_MAX/2) : ((unsigned int)count);
  unsigned char const* data = (unsigned char const*)buffer;
#if defined(Reedkiln_Atomic)
  unsigned int const swap_index = Reedkiln_Atomic_Get(&reedkiln_log_index)%2u;
#else
  unsigned int const swap_index = reedkiln_log_index%2u;
#endif /*Reedkiln_Atomic*/
  struct reedkiln_logbuf* const ptr = reedkiln_log_buffers+swap_index;
  unsigned int const src = reedkiln_log_nextpos(ptr, count);
  if (src == UINT_MAX)
    return 0;
  /* write (possibly truncated) content to log buffer */{
    size_t const put_length =
      (size_t)((reedkiln_log_size - src < n) ? reedkiln_log_size - src : n);
    memcpy(ptr->data + src, buffer, put_length);
    return put_length;
  }
}

reedkiln_size reedkiln_log_printf(char const* format, ...) {
  unsigned int count;
#if defined(Reedkiln_Atomic)
  unsigned int const swap_index = Reedkiln_Atomic_Get(&reedkiln_log_index)%2u;
#else
  unsigned int const swap_index = reedkiln_log_index%2u;
#endif /*Reedkiln_Atomic*/
  struct reedkiln_logbuf* const ptr = reedkiln_log_buffers+swap_index;
  unsigned int src;
  /* calculate size */{
    va_list ap;
    int len;
    va_start(ap, format);
    len = reedkiln_log_vsnprintf(NULL, 0, format, ap);
    if (len == -1)
      return 0;
    va_end(ap);
    count = len+1u;
  }
  /* allocate buffer */{
    src = reedkiln_log_nextpos(ptr, count);
    if (src == UINT_MAX)
      return 0;
  }
  /* write (possibly truncated) content to log buffer */{
    size_t const put_length = (size_t)((reedkiln_log_size - src < count)
       ? reedkiln_log_size - src : count);
    va_list ap;
    va_start(ap, format);
    (void)reedkiln_log_vsnprintf(ptr->data+src, put_length, format, ap);
    va_end(ap);
    return (unsigned int)put_length;
  }
}

struct reedkiln_log_attr reedkiln_log_adjust_type(char const** p) {
  struct reedkiln_log_attr value = {Reedkiln_Bias};
  /* skip over the initial % */
  (*p) += 1;
  /* skip flags */
  for (; **p && memchr("0-+ #",**p,5); (*p) += 1)
    continue;
  /* width */
  if (**p == '*') {
    value.width = 255;
    (*p) += 1;
  } else for (; **p >= '0' && **p <= '9'; (*p) += 1)
    value.width = (value.width * 10u + (**p - '0'))&127u;
  if (**p == '.') {
    (*p) += 1;
    if (**p == '*') {
      value.prec = 255;
      (*p) += 1;
    } else for (; **p >= '0' && **p <= '9'; (*p) += 1)
      value.prec = (value.prec * 10u + (**p - '0'))&127u;
  }
  /* handle attributes */
  for (; **p; (*p) += 1) {
    char const ch = **p;
    if (ch == 'h')
      value.value -= 1;
    else if (ch == 'l')
      value.value += 1;
    else if (ch == 'z')
      value.value = 5 + Reedkiln_Bias;
    else if (ch == 'L')
      value.value = 10 + Reedkiln_Bias;
    else
      break;
  }
  return value;
}

int reedkiln_log_itox
  (unsigned char* output, reedkiln_intmax value)
{
  char const xdigits[] = "0123456789abcdef";
  int i;
  if (value == 0) {
    output[0] = '0';
    return 1;
  }
  for (i = 0; value > 0 && i < Reedkiln_ItoXMax; value >>= 4, ++i) {
    output[i] = xdigits[value&15u];
  }
  return i;
}
int reedkiln_log_itoa
  (unsigned char* output, reedkiln_intmax value)
{
  int i;
  if (value == 0) {
    output[0] = '0';
    return 1;
  }
  for (i = 0; value > 0 && i < Reedkiln_ItoAMax; value /= 10, ++i) {
    output[i] = (value%10u) + '0';
  }
  return i;
}
int reedkiln_log_dtox
  (unsigned char* output, long double value)
{
  char const xdigits[] = "0123456789abcdef";
  int i = 0;
  int expon = 0;
  if (value == 0.0) {
    output[0] = '0';
    return 1;
  }
  /* find the exponent */
  if (value < 1.0) {
    for (expon = 0; expon > Reedkiln_DMinExp; expon -= 4) {
      if (value >= 1.0)
        break;
      else
        value *= 16.0;
    }
  } else {
    for (expon = 0; expon < Reedkiln_DMaxExp; expon += 4) {
      if (value < 16.0)
        break;
      else
        value *= 0.0625;
    }
  }
  for (i = 0; value > 0.0 && i < (LDBL_DIG); ++i) {
    int const digit = (int)value;
    value = (value - digit)*16.0;
    output[i] = xdigits[digit&15u];
    if (i == 0 && value > 0.0)
      output[++i] = '.';
  }
  if (expon != 0) {
    int const expsign = (expon < 0);
    int const expstart = i;
    i += reedkiln_log_itoa(output+i, abs(expon));
    if (expsign)
      output[i++] = '-';
    output[i++] = 'p';
    /* reverse */{
      int const half = (i - expstart)/2;
      int j;
      for (j = 0; j < half; ++j) {
        unsigned char const tmp = output[expstart + j];
        output[expstart + j] = output[i - j - 1];
        output[i - j - 1] = tmp;
      }
    }
  }
  return i;
}

unsigned int reedkiln_log_count_ncat
  ( unsigned char* output, size_t sz, unsigned count,
    unsigned precision, char const* str)
{
  unsigned int j;
  for (j = 0; j < precision && str[j]; ++j) {
    if (count < sz)
      output[count] = str[j];
    count += 1;
  }
  return count;
}
unsigned int reedkiln_log_count_str
  (unsigned char* output, size_t sz, unsigned count, char const* str)
{
  for (; *str; ++str) {
    if (count < sz)
      output[count] = *str;
    count += 1;
  }
  return count;
}
unsigned int reedkiln_log_count_strn
  ( unsigned char* output, size_t sz, unsigned count,
    unsigned int slen, char const* str)
{
  unsigned int i;
  for (i = 0; i < slen; ++i) {
    if (count < sz)
      output[count] = str[i];
    count += 1;
  }
  return count;
}

unsigned int reedkiln_log_count_revn
  ( unsigned char* output, size_t sz, unsigned count,
    int slen, unsigned char const* str)
{
  int j;
  for (j = slen-1; j >= 0; --j) {
    if (count < sz)
      output[count] = str[j];
    count += 1;
  }
  return count;
}

int reedkiln_log_vsnprintf
  (unsigned char* output, size_t sz, char const* format, va_list ap)
{
  unsigned int count = 0;
#if (defined __STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  count = vsnprintf((char*)output, sz, format, ap);
#else
  char const* p;
  for (p = format; *p; ++p) {
    if (*p == '%') {
      struct reedkiln_log_attr attr = reedkiln_log_adjust_type(&p);
      int const type_adjust = (int)(attr.value - Reedkiln_Bias);
      unsigned int precision = 0;
      if (!*p)
        break;
      if (attr.width == 255u)
        (void)va_arg(ap, int);
      if (attr.prec == 255u) {
        int const prec = va_arg(ap, int);
        precision = (prec > 0 ? prec : 0);
      } else precision = attr.prec;
      switch (*p) {
      case 's':
        if (type_adjust > 0) {
          wchar_t const* str = va_arg(ap, wchar_t const*);
          unsigned char utf8s[Reedkiln_UTF8Max] = {0};
          unsigned int j = 0;
          if (!str) {
            count = reedkiln_log_count_str(output, sz, count, "(null)");
          } else for (; *str && (!precision || j < precision); ++str, ++j) {
            wchar_t ch = *str;
            int utf8len = 0;
            /* assume utf-8 for a simpler re-entrant algorithm */{
              unsigned int last_mask = 0x1F;
              if (ch <= 0x7f) {
                utf8s[0] = (unsigned char)ch;
                utf8len = 1;
              } else {
                for (utf8len = 0; utf8len < 6 && ch > last_mask;
                  last_mask >>= 1, ++utf8len, ch >>= 6)
                {
                  utf8s[utf8len] = (0x80 | (ch & 0x3f));
                }
                if (utf8len < 6) {
                  utf8s[utf8len] = ((~last_mask)<<2) | ch;
                  utf8len += 1u;
                } else {
                  utf8s[0] = 0xBD;
                  utf8s[1] = 0xBF;
                  utf8s[2] = 0xEF;
                  utf8len = 3;
                }
              }
            }
            count = reedkiln_log_count_revn
                (output, sz, count, utf8len, utf8s);
          }
        } else {
          char const* str = va_arg(ap, char const*);
          if (precision && str)
            count = reedkiln_log_count_ncat
                (output, sz, count, precision, str?str:"(null)");
          else
            count = reedkiln_log_count_str(output, sz, count, str?str:"(null)");
        } break;
      case 'p':
        if (sizeof(void*) > sizeof(reedkiln_intmax)) {
          void const* const value = va_arg(ap, void const*);
          char const* str = (value ? "(nonnull)" : "(null)");
          count = reedkiln_log_count_str(output, sz, count, str);
        }
        /* [[fallthrough]] */;
      case 'X':
      case 'x':
        {
          reedkiln_intmax value;
          unsigned char ibuffer[Reedkiln_ItoXMax];
          int ilen;
          if (*p == 'p')
            value = (reedkiln_intmax)va_arg(ap, void const*);
          else if (type_adjust <= -2)
            value = va_arg(ap, unsigned int)&UCHAR_MAX;
          else if (type_adjust == -1)
            value = va_arg(ap, unsigned int)&USHRT_MAX;
          else if (type_adjust == 0)
            value = va_arg(ap, unsigned int);
          else if (type_adjust == 1)
            value = va_arg(ap, unsigned long);
          else if (type_adjust >= 5)
            value = (reedkiln_intmax)va_arg(ap, size_t);
          else if (type_adjust >= 2) {
#if defined(ULLONG_MAX)
            value = va_arg(ap, unsigned long long);
#else
            /* uh oh */
            count = UINT_MAX;
            break;
#endif /*ULLONG_MAX*/
          }
          ilen = reedkiln_log_itox(ibuffer, value);
          count = reedkiln_log_count_revn(output, sz, count, ilen, ibuffer);
        } break;
      case 'i':
      case 'd':
        {
          reedkiln_intmax value;
          reedkiln_intmax valuemax;
          unsigned char ibuffer[Reedkiln_ItoXMax];
          int ilen;
          if (type_adjust <= -2) {
            value = va_arg(ap, unsigned int)&UCHAR_MAX;
            valuemax = (reedkiln_intmax)(UCHAR_MAX);
          } else if (type_adjust == -1) {
            value = va_arg(ap, unsigned int)&USHRT_MAX;
            valuemax = (reedkiln_intmax)(USHRT_MAX);
          } else if (type_adjust == 0) {
            value = va_arg(ap, unsigned int);
            valuemax = (reedkiln_intmax)(UINT_MAX);
          } else if (type_adjust == 1) {
            value = va_arg(ap, unsigned long);
            valuemax = (reedkiln_intmax)(ULONG_MAX);
          } else if (type_adjust >= 5) {
            value = (reedkiln_intmax)va_arg(ap, size_t);
            valuemax = (reedkiln_intmax)(~(size_t)0);
          } else if (type_adjust >= 2) {
#if defined(ULLONG_MAX)
            value = va_arg(ap, unsigned long long);
            valuemax = (ULLONG_MAX);
#else
            /* uh oh */
            count = UINT_MAX;
            break;
#endif /*ULLONG_MAX*/
          }
          if (value & ~(valuemax>>1)) {
            value = ((~value) + 1u) & valuemax;
            count = reedkiln_log_count_strn(output, sz, count, 1,"-");
          }
          ilen = reedkiln_log_itoa(ibuffer, value);
          count = reedkiln_log_count_revn(output, sz, count, ilen, ibuffer);
        } break;
      case 'a':
      case 'A':
        {
          long double value;
          unsigned char dbuffer[Reedkiln_DtoXMax];
          int dlen = 0;
          char sign = 0;
          char prefix = 1;
          if (type_adjust >= 10)
            value = va_arg(ap, long double);
          else
            value = va_arg(ap, double);
          if (value != value) {
            memcpy(dbuffer, "nan", 3);
            dlen = 3;
            prefix = 0;
          } else {
            if (value < 0) {
              sign = 1;
              value = -value;
            }
            if (value/(FLT_RADIX) == value && value > 0.0) {
              memcpy(dbuffer, "inf", 3);
              dlen = 3;
              prefix = 0;
            } else {
              dlen = reedkiln_log_dtox(dbuffer, value);
            }
          }
          /* post the bytes */{
            int j;
            if (sign)
              count = reedkiln_log_count_strn(output, sz, count, 1,"-");
            if (prefix)
              count = reedkiln_log_count_strn(output, sz, count, 2,"0x");
            count = reedkiln_log_count_strn
              (output, sz, count, dlen, (char*)dbuffer);
          }
        } break;
      case '%':
        count = reedkiln_log_count_strn(output, sz, count, 1, "%");
        break;
      default:
#if (defined EINVAL)
        errno = EINVAL;
#else
        errno = EDOM;
#endif /*EINVAL*/
        return -1;
      }
    } else {
      count = reedkiln_log_count_strn(output, sz, count, 1,p);
    }
  }
  if (count < sz)
    output[count] = '\0';
  else if (sz)
    output[sz-1u] = '\0';
#endif
  return (count >= INT_MAX) ? -1 : (int)count;
}

void reedkiln_log_escape(unsigned char const* data, size_t n, FILE* f) {
  size_t i;
  int was_digit = 0;
  for (i = 0; i < n; ++i) {
    unsigned char const ch = data[i];
    int const previous_digit = was_digit;
    if (!ch)
      continue;
    was_digit = 0;
    if (ch == '"')
      fputs("\\\"", f);
    else if (previous_digit && isxdigit(ch)) {
      fprintf(f, "\\x%02x", ch&255u);
      was_digit = 1;
    } else if (isgraph(ch))
      fputc(ch, f);
    else switch (ch) {
    case ' ':
      fputc(' ', f); break;
    case '\n':
      fputs("\\n", f); break;
    case '\t':
      fputs("\\t", f); break;
    case '\b':
      fputs("\\b", f); break;
    case '\f':
      fputs("\\f", f); break;
    case '\v':
      fputs("\\v", f); break;
    case '\a':
      fputs("\\a", f); break;
    default:
      fprintf(f, "\\x%02x", ch&255u); was_digit = 1; break;
    }
  }
}
/* END   log buffer */

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
  struct reedkiln_vtable const initial_table = reedkiln_vtable_c;
  reedkiln_next_status = Reedkiln_OK;
  reedkiln_bail_status = Reedkiln_OK;
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
    int want_skip_tf = (!reedkiln_prefix_match(test->name, testname_prefix));
    int skip_tf = ((test->flags & Reedkiln_SKIP)!= 0)
        || want_skip_tf;
    int res = 0;
    if (want_skip_tf) {
      direct_text = " # SKIP by request";
    } else if (!skip_tf) {
      struct reedkiln_box const* box = test->box;
      void* box_item = NULL;
      int box_called = 0;
      unsigned int log_index;
      reedkiln_srand(rand_seed);
      reedkiln_log_reset();
      reedkiln_vtable_c = initial_table;
      if (box != NULL && box->setup != NULL) {
        res = reedkiln_run_setup(box->setup, p, &box_item);
        if (reedkiln_bail_status != Reedkiln_OK) {
          total_res = EXIT_FAILURE;
          break;
        } else if (res != Reedkiln_OK) {
          box_called = 2;
        } else box_called = 1;
      }
      if (box_called <= 1)
        res = reedkiln_run_test(test->cb, box_called ? box_item : p);
      if (box_called == 1 && box->teardown != NULL) {
        (*box->teardown)(box_item);
      }
    }
    if (reedkiln_bail_status != Reedkiln_OK) {
      total_res = EXIT_FAILURE;
      break;
    }
    switch (res) {
    case 0:
      result_text = "ok"; break;
    case Reedkiln_IGNORE:
      result_text = "ok";
      direct_text = " # SKIP at runtime";
      break;
    default:
      if (!(test->flags & Reedkiln_TODO))
        total_res = EXIT_FAILURE;
      result_text = "not ok";break;
    }
    fprintf(stdout, "%s %lu - %s%s\n",
      result_text, ((unsigned long int)(test_i+1)), t[test_i].name,
      direct_text);
    /* render the log */if (!skip_tf) {
      unsigned int const log_index = reedkiln_log_swap();
      struct reedkiln_logbuf const* const ptr = reedkiln_log_buffers+log_index;
#if defined(Reedkiln_Atomic)
      unsigned int const log_pos = Reedkiln_Atomic_Get(&ptr->pos);
#else
      unsigned int const log_pos = ptr->pos;
#endif /*Reedkiln_Atomic*/
      if (log_pos > 0) {
        fputs("  ---\n  message: \"", stdout);
        reedkiln_log_escape(ptr->data, log_pos, stdout);
        fputs("\"\n  ...\n", stdout);
      }
    }
  }
  return total_res;
}
