#include "../reedkiln.h"
#include <stdexcept>
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

int test_expect(void*);
int test_cancel(void*);
int test_zeta(void*);

struct reedkiln_entry tests[] = {
  { "expect", test_expect, 0, &reedkiln::expect_box<void,
        reedkiln::cxx_accept<std::out_of_range>,
        reedkiln::cxx_accept<std::range_error>
      >::value },
  { "cancel", test_cancel, Reedkiln_TODO, &reedkiln::expect_box<void,
        reedkiln::cxx_reject<std::out_of_range>,
        reedkiln::cxx_reject<std::range_error>
      >::value },
  { "zeta", test_zeta },
  { NULL, NULL }
};

/* test exception expecter */
int test_expect(void* p) {
  free_box box(2);
  if (reedkiln_rand() % 1 > 0) {
    throw std::out_of_range("expect 1");
  } else {
    throw std::range_error("expect 0");
  }
  return Reedkiln_NOT_OK;
}

/* test exception canceller's ability to avoid forced `abort()` */
int test_cancel(void* p) {
  free_box box(2);
  if (reedkiln_rand() % 1 > 0) {
    throw std::out_of_range("cancel 1");
  } else {
    throw std::range_error("cancel 0");
  }
  return Reedkiln_OK;
}

/* last test to run */
int test_zeta(void* p) {
  return Reedkiln_OK;
}


int main(int argc, char **argv) {
  return reedkiln::cxx_main(tests, argc, argv, 0);
}
