/* SPDX-License-Identifier: Unlicense */
/**
 * @file reedkiln.h
 * @brief Short test header.
 */
#if !defined(hg_Reedkiln_reedkiln_h_)
#define hg_Reedkiln_reedkiln_h_

#if defined(__cplusplus)
#  include <exception>
#  include <cstddef>
  typedef std::size_t reedkiln_size;
#else
#  include <stddef.h>
  typedef size_t reedkiln_size;
#endif /*__cplusplus*/


#if defined(__cplusplus)
extern "C" {
#endif /*__cplusplus*/

/**
 * @brief Testing code.
 * @param p either a box item (if setup is provided) or userdata from main
 * @return a @link reedkiln_result @endlink value
 */
typedef int (*reedkiln_cb)(void* p);

/**
 * @brief Teardown code.
 * @param p box item to destroy and free
 */
typedef void (*reedkiln_teardown_cb)(void* p);
/**
 * @brief Setup code.
 * @param p user data from main
 * @return a box item on success, NULL otherwise
 */
typedef void* (*reedkiln_setup_cb)(void*);

enum reedkiln_flag {
  Reedkiln_ZERO = 0,
  Reedkiln_TODO = 1,
  Reedkiln_SKIP = 2
};

struct reedkiln_box {
  reedkiln_setup_cb setup;
  reedkiln_teardown_cb teardown;
};
typedef struct reedkiln_box reedkiln_box;

struct reedkiln_entry {
  char const* name;
  reedkiln_cb cb;
  unsigned int flags;
  /**
   * @brief Setup and teardown callbacks.
   */
  struct reedkiln_box const* box;
};
typedef struct reedkiln_entry reedkiln_entry;

enum reedkiln_result {
  Reedkiln_OK = 0,
  Reedkiln_NOT_OK = 1,
  Reedkiln_THROWS = 2
};

/**
 * @brief Catch callback.
 * @param cb the possibly-throwing test function to call
 * @param ptr input to pass to the callbacks
 * @return the result from the test function, or
 *   Reedkiln_THROWS if the function threw a catchable
 *   exception
 */
typedef int (*reedkiln_catch_cb)(reedkiln_cb cb, void* ptr);
/**
 * @brief Fail fast callback.
 * @noreturn
 */
typedef void (*reedkiln_fail_cb)(void);

struct reedkiln_vtable {
  reedkiln_catch_cb catch_cb;
  reedkiln_fail_cb fail_cb;
};

/**
 * @brief Get a random number.
 */
unsigned int reedkiln_rand(void);
/**
 * @brief Fill a buffer with random bytes.
 */
void reedkiln_memrand(void* b, reedkiln_size sz);

/**
 * @brief Run some tests.
 * @param t array of tests
 * @param argc from `main`
 * @param argv from `main`
 * @param p to pass to the callbacks
 * @return an exit code
 */
int reedkiln_main
    (struct reedkiln_entry const* t, int argc, char **argv, void* p);

/**
 * @brief Configure the failure code path.
 * @param v virtual table with custom code path
 */
void reedkiln_set_vtable(struct reedkiln_vtable const* vt);

/**
 * @brief Fail the current test.
 */
#if (defined __cplusplus) && (__cplusplus >= 201103L)
[[noreturn]]
#elif (defined __STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Noreturn
#endif /*__cplusplus || __STDC_VERSION__*/
void reedkiln_fail(void);

/**
 * @brief Bail out from all tests.
 * @param reason reason for bailing on tests
 */
#if (defined __cplusplus) && (__cplusplus >= 201103L)
[[noreturn]]
#elif (defined __STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Noreturn
#endif /*__cplusplus || __STDC_VERSION__*/
void reedkiln_bail_out(char const* reason);

#if defined(__cplusplus)
};
#endif /*__cplusplus*/

#if defined(__cplusplus)
namespace reedkiln {
  class cxx_failure;
  int cxx_catcher(::reedkiln_cb cb, void* ptr);
  void cxx_fail(void);
  void cxx_set_vtable();
  int cxx_main
    (struct reedkiln_entry const* t, int argc, char **argv, void* p);


  class cxx_failure
#  if __cplusplus >= 201103L
    final
#  endif /*__cplusplus*/
    : public std::exception {
  public:
    cxx_failure() : std::exception() {}
    char const* what() const
#  if __cplusplus >= 201103L
        noexcept
#  else
        throw()
#  endif /*__cplusplus*/
    {
      return "A test callback reports \"not ok\".";
    }
  };

  inline
  int cxx_catcher(::reedkiln_cb cb, void* ptr) {
    try {
      return (*cb)(ptr);
    } catch (cxx_failure const& ) {
      return Reedkiln_NOT_OK;
    } catch (...) {
      /* let the tester break and */throw;
    }
  }

  inline
  void cxx_fail(void) {
    throw cxx_failure();
  }

  inline
  void cxx_set_vtable() {
    struct reedkiln_vtable const vt = { &cxx_catcher, &cxx_fail };
    return reedkiln_set_vtable(&vt);
  }

  inline
  int cxx_main
    (struct reedkiln_entry const* t, int argc, char **argv, void* p)
  {
    ::reedkiln::cxx_set_vtable();
    return ::reedkiln_main(t, argc, argv, p);
  }
};
#endif /*__cplusplus*/

#endif /*hg_Reedkiln_reedkiln_h_*/
