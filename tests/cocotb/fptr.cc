#pragma GCC optimize("O0")

#include <cstdint>
#include <cstring>

typedef void *(*CopyFn)(void *, const void *, size_t);

int main(int argc, char **argv) {
  uint32_t src = 0xdeadbeef;
  uint32_t dst = 0;
  CopyFn m = memcpy;
  m(&dst, &src, sizeof(src));
  return (src == dst) ? 0 : 1;
}