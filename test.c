#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define regist_test_case(func)              \
  printf("test case: %s start..\n", #func); \
  func();                                   \
  printf("test case: %s end..\n", #func);

// ---------------------------test case 1---------------------------------------

static inline size_t get_next_block_size(size_t size) {
  const static int ALIGN = 32;
  const static size_t max = 1 << 14;  // max = 16K

  size_t n = ((size + ALIGN - 1) & ~(ALIGN - 1));
  return n >= max ? max : n;
}

static inline size_t next_align(size_t size, size_t align) {
  assert(size >= 0);
  assert(align > 0);
  do {
    if (size % align == 0) return size;
    size++;
  } while (true);
}

void test_case1() {
  size_t n1, n2;
  for (int i = 1; i <= 1024; i++) {
    n1 = get_next_block_size(i);
    n2 = next_align(i, 32);
    //printf("%d -> %d\n", i, n1);
    assert(n1 == n2);
  }
}

// ------------------------------------------------------------------------

int main() {
  regist_test_case(test_case1);
  return 0;
}