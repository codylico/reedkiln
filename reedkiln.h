/* SPDX-License-Identifier: Unlicense */
/**
 * @file reedkiln.h
 * @brief Short test header.
 */
#if !defined(hg_Reedkiln_reedkiln_h_)
#define hg_Reedkiln_reedkiln_h_

#if (defined __cplusplus)
#  if (__cplusplus >= 201103L)
#    define Reedkiln_Noexcept noexcept
#  else
#    define Reedkiln_Noexcept throw()
#  endif /*__cplusplus>=201103L*/
#else
#  define Reedkiln_Noexcept
#endif /*__cplusplus*/

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
  /** @brief "ok" code from TAP. */
  Reedkiln_OK = 0,
  /** @brief "not ok" code from TAP. */
  Reedkiln_NOT_OK = 1,
  Reedkiln_THROWS = 2,
  /** @brief Count this run as a skip at runtime. */
  Reedkiln_IGNORE = 3
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
 * @brief Report a fail if a condition is false (zero).
 * @param val zero to fail, nonzero otherwise
 * @param text a description of what (may have) failed
 * @param file name of source file
 * @param line line number of related source code
 * @note Calls reedkiln_fail.
 */
void reedkiln_assert_ex
  (int val, char const* text, char const* file, unsigned long int line);

#define reedkiln_assert(x) \
    reedkiln_assert_ex(((x)?1:0), #x, __FILE__, __LINE__)

/**
 * @brief Run some tests.
 * @param t array of tests
 * @param argc from `main`
 * @param argv from `main`
 * @param p to pass to the callbacks
 * @return an exit code
 * @note This library assumes at most one `reedkiln_main` per process
 *   is running at a time.
 */
int reedkiln_main
    (struct reedkiln_entry const* t, int argc, char **argv, void* p)
#if (defined __cplusplus) && (__cplusplus >= 201103L)
    noexcept(false)
#endif /*__cplusplus*/
    ;

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

  class cxx_failure : public std::exception {
  public:
    cxx_failure() : std::exception() {}
    char const* what() const Reedkiln_Noexcept {
      return "A test callback reports \"not ok\".";
    }
  };

  template <typename t>
  struct cxx_box {
  public:
    using type = t;
    static void* setup(void*) {
      return new t;
    }
    static void teardown(void* p) Reedkiln_Noexcept {
      delete static_cast<t*>(p);
    }
    static reedkiln_box const value;
    static reedkiln_box const* const ptr;
  };
  template <typename t>
  reedkiln_box const cxx_box<t>::value = { &setup, &teardown };
  template <typename t>
  reedkiln_box const* const cxx_box<t>::ptr = &cxx_box<t>::value;

#  if (defined Reedkiln_UseExpect) || (__cplusplus >= 201103L)
  /**
   * @brief Allow the exception and report success.
   * @tparam ExceptionType type of exception to allow
   */
  template <typename ExceptionType>
  class cxx_accept {
    using type = ExceptionType;
  };
  /**
   * @brief Catch the exception and report fail-by-exception.
   * @tparam ExceptionType type of exception to catch
   */
  template <typename ExceptionType>
  class cxx_catch {
    using type = ExceptionType;
  };
  /**
   * @brief Reject the exception and report failure-by-throw.
   * @tparam ExceptionType type of exception to reject
   */
  template <typename ExceptionType>
  class cxx_reject {
    using type = ExceptionType;
  };

  template <typename Exception>
  class cxx_subcatcher {
  private:
    template <typename t>
    struct unsupported {
      static constexpr bool value = false;
    };
  public:
    static signed char check(std::exception_ptr ep) noexcept {
      static_assert(unsupported<Exception>::value,
          "Wrap your exception type in cxx_catch<t>,"
          " cxx_accept<t>, or cxx_reject<t>.");
      reedkiln_bail_out("Unhandled exception type.");
      /* [[unreachable]] */return -1;
    }
  };
  template <typename Exception>
  class cxx_subcatcher< cxx_catch<Exception> > {
  public:
    static signed char check(std::exception_ptr ep) noexcept {
      try {
        std::rethrow_exception(ep);
      } catch (Exception const&) {
        return Reedkiln_THROWS;
      } catch (...) {
        return -1;
      }
      /* [[unreachable]] */return -1;
    }
  };

  template <typename Accepted>
  class cxx_subcatcher< cxx_accept<Accepted> > {
  public:
    static signed char check(std::exception_ptr ep) noexcept {
      try {
        std::rethrow_exception(ep);
      } catch (Accepted const&) {
        return Reedkiln_OK;
      } catch (...) {
        return -1;
      }
      /* [[unreachable]] */return -1;
    }
  };

  template <typename Rejected>
  class cxx_subcatcher< cxx_reject<Rejected> > {
  public:
    static signed char check(std::exception_ptr ep) noexcept {
      try {
        std::rethrow_exception(ep);
      } catch (Rejected const&) {
        return Reedkiln_NOT_OK;
      } catch (...) {
        return -1;
      }
      /* [[unreachable]] */return -1;
    }
  };

  template <typename... Exceptions>
  class cxx_expecter {
  public:
    static int catcher(::reedkiln_cb cb, void* ptr) {
      try {
        return cxx_catcher(cb, ptr);
      } catch (...) {
        std::exception_ptr ep = std::current_exception();
        signed char results[sizeof...(Exceptions)] =
            { cxx_subcatcher<Exceptions>::check(ep) ... };
        std::size_t i;
        for (i = 0; i < sizeof...(Exceptions); ++i) {
          int const res = results[i];
          if (res >= 0)
            /* exception safely recognized, so */return res;
        }
        /* don't hide the exception; */std::rethrow_exception(ep);
      }
    }
    static void set_vtable() noexcept {
      struct reedkiln_vtable const vt = { &catcher, &cxx_fail };
      return reedkiln_set_vtable(&vt);
    }
  };

  template <typename t, typename... Exceptions>
  struct expect_box {
    using type = t;
    static void* setup(void*) {
      cxx_expecter<Exceptions...>::set_vtable();
      return new t;
    }
    static reedkiln_box const value;
    static constexpr reedkiln_box const* ptr = &value;
  };
  template <typename... Exceptions>
  struct expect_box<void, Exceptions...> {
    using type = void;
    static void* setup(void* p) {
      cxx_expecter<Exceptions...>::set_vtable();
      return p;
    }
    static reedkiln_box const value;
    static constexpr reedkiln_box const* ptr = &value;
  };

  template <typename t, typename... Exceptions>
  reedkiln_box const expect_box<t, Exceptions...>::value =
    { &expect_box<t, Exceptions...>::setup, &cxx_box<t>::teardown };
  template <typename... Exceptions>
  reedkiln_box const expect_box<void, Exceptions...>::value =
    { &expect_box<void, Exceptions...>::setup, 0 };
#  endif /*Reedkiln_UseExpect*/


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
