#include "bench.h"

#include <sys/nearptr.h>

#include <cstring>
#include <cstdio>

namespace {

typedef unsigned char u8;

template <unsigned CopySize>
FORCEINLINE void copy_c_loop(const u8 * __restrict__ src, int size, u8 * __restrict__ dst) {
  // Read word size at a time for performance.
  STATIC_ASSERT(sizeof(unsigned) == 4);
  STATIC_ASSERT((CopySize % 4) == 0);
  unsigned const *in = (unsigned const*)src;
  unsigned *out = (unsigned*)dst;

  // Unroll for even more performance.
#define COPY_UNROLL_FACTOR 16
#define COPY_ONE(o) out[i + o] = in[i + o];
  STATIC_ASSERT(((CopySize / 4) % COPY_UNROLL_FACTOR) == 0);
  for (int i = 0; i < size / 4; i += COPY_UNROLL_FACTOR) {
    COPY_ONE(0)  COPY_ONE(1)  COPY_ONE(2)  COPY_ONE(3)
    COPY_ONE(4)  COPY_ONE(5)  COPY_ONE(6)  COPY_ONE(7)
    COPY_ONE(8)  COPY_ONE(9)  COPY_ONE(10) COPY_ONE(11)
    COPY_ONE(12) COPY_ONE(13) COPY_ONE(14) COPY_ONE(15)
  }
}

FORCEINLINE void copy_asm(const u8 * __restrict__ src, int size, u8 * __restrict__ dst) {
  // Read word size at a time for performance.
  STATIC_ASSERT(sizeof(unsigned) == 4);
  //STATIC_ASSERT((CopySize % 4) == 0);
  unsigned const *in = (unsigned const*)src;
  unsigned *out = (unsigned*)dst;

  // Stolen from ___dj_movedata, but with the outer bits removed.
  __asm__ __volatile__ (
    "cld\n"
    // gcc is too old to let me do this directly
    "mov %0, %%edi\n"
    "mov %1, %%esi\n"
    "mov %2, %%ecx\n"
    "rep movsl\n"
    : // no outputs
    : "r"(out), "r"(in), "r"(size / 4)
    : "memory", "edi", "esi", "ecx"
  );
}

FORCEINLINE void copy_memcpy(const u8 * __restrict__ src, int size, u8 * __restrict__ dst) {
  memcpy(dst, src, size);
}

} // namespace


BENCHMARK_FUNC(compare_memcpy) {
  // Grab a pointer to vram.
  if (__djgpp_nearptr_enable() == 0) {
    printf("Can't enable nearpt. Skipping memcpy benchmark\n");
    return;
  }
  u8 *vram = (u8*)(__djgpp_conventional_base + 0xA8000);

  const int num_iterations = 10000;
  ALIGNAS(4) u8 big_buffer[640];

  // Read
  BENCHMARK_RUN(c_loop_64_read, num_iterations) {
    copy_c_loop<64>(vram, 640, big_buffer);
  }
  BENCHMARK_RUN(c_loop_128_read, num_iterations) {
    copy_c_loop<128>(vram, 640, big_buffer);
  }
  BENCHMARK_RUN(c_loop_640_read, num_iterations) {
    copy_c_loop<640>(vram, 640, big_buffer);
  }
  BENCHMARK_RUN(asm_loop_read, num_iterations) {
    copy_asm(vram, 640, big_buffer);
  }
  BENCHMARK_RUN(memcpy_loop_read, num_iterations) {
    copy_memcpy(vram, 640, big_buffer);
  }

  // Write
  BENCHMARK_RUN(c_loop_64_write, num_iterations) {
    copy_c_loop<64>(big_buffer, 640, vram);
  }
  BENCHMARK_RUN(c_loop_128_write, num_iterations) {
    copy_c_loop<128>(big_buffer, 640, vram);
  }
  BENCHMARK_RUN(c_loop_640_write, num_iterations) {
    copy_c_loop<640>(big_buffer, 640, vram);
  }
  BENCHMARK_RUN(asm_loop_write, num_iterations) {
    copy_asm(big_buffer, 640, vram);
  }
  BENCHMARK_RUN(memcpy_loop_write, num_iterations) {
    copy_memcpy(big_buffer, 640, vram);
  }

  // Revert back.
  __djgpp_nearptr_disable();
}
