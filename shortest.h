/**
 * @file shortest.h
 * @brief Short test header.
 */
#if defined(__cplusplus)
#  include <cstddef>
  typedef std::size_t shortest_size;
#else
#  include <stddef.h>
  typedef size_t shortest_size;
#endif /*__cplusplus*/

#if defined(__cplusplus)
extern "C" {
#endif /*__cplusplus*/

typedef int (*shortest_cb)(void*);

enum shortest_flag {
  Shortest_ZERO = 0,
  Shortest_TODO = 1,
  Shortest_SKIP = 2
};

typedef struct shortest_entry {
    char const* name;
    shortest_cb cb;
    int flags;
  } shortest_entry;

enum shortest_result {
  Shortest_OK = 0,
  Shortest_NOT_OK = 1
};

/**
 * @brief Get a random number.
 */
unsigned int shortest_rand(void);
/**
 * @brief Fill a buffer with random bytes.
 */
void shortest_memrand(void* b, shortest_size sz);

/**
 * @brief Run some tests.
 * @param t array of tests
 * @param argc from `main`
 * @param argv from `main`
 * @param p to pass to the callbacks
 * @return an exit code
 */
int shortest_main
    (struct shortest_entry const* t, int argc, char **argv, void* p);

#if defined(__cplusplus)
};
#endif /*__cplusplus*/
