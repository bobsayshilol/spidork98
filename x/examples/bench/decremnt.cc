//
// Similar to the stuff in gpuscrn.cc, but with cycles on alternating
// columns so that it doesn't all go to 0.
//

#include "bench.h"

namespace {

typedef unsigned char u8;
#define GPU_WIDTH 640

u8 decrement_if_positive_lookup[256];

FORCEINLINE void basic_impl(u8 (&scanline_data)[GPU_WIDTH]) {
  for (int x = 0; x < GPU_WIDTH; x++) {
    if (scanline_data[x] >= 1) {
      scanline_data[x] -= 2;
    }
  }
}

FORCEINLINE void unrolled_impl_8(u8 (&scanline_data)[GPU_WIDTH]) {
  for (int x = 0; x < GPU_WIDTH; x += 8) {
#define DECREMENT_IF_POSITIVE(o) \
{ \
  u8 &pixel = scanline_data[(x) + (o)]; \
  pixel = decrement_if_positive_lookup[pixel]; \
}
    DECREMENT_IF_POSITIVE(0)  DECREMENT_IF_POSITIVE(1)  DECREMENT_IF_POSITIVE(2)  DECREMENT_IF_POSITIVE(3)
    DECREMENT_IF_POSITIVE(4)  DECREMENT_IF_POSITIVE(5)  DECREMENT_IF_POSITIVE(6)  DECREMENT_IF_POSITIVE(7)
  }
}

FORCEINLINE void unrolled_impl_16(u8 (&scanline_data)[GPU_WIDTH]) {
  for (int x = 0; x < GPU_WIDTH; x += 16) {
#define DECREMENT_IF_POSITIVE(o) \
{ \
  u8 &pixel = scanline_data[(x) + (o)]; \
  pixel = decrement_if_positive_lookup[pixel]; \
}
    DECREMENT_IF_POSITIVE(0)  DECREMENT_IF_POSITIVE(1)  DECREMENT_IF_POSITIVE(2)  DECREMENT_IF_POSITIVE(3)
    DECREMENT_IF_POSITIVE(4)  DECREMENT_IF_POSITIVE(5)  DECREMENT_IF_POSITIVE(6)  DECREMENT_IF_POSITIVE(7)
    DECREMENT_IF_POSITIVE(8)  DECREMENT_IF_POSITIVE(9)  DECREMENT_IF_POSITIVE(10) DECREMENT_IF_POSITIVE(11)
    DECREMENT_IF_POSITIVE(12) DECREMENT_IF_POSITIVE(13) DECREMENT_IF_POSITIVE(14) DECREMENT_IF_POSITIVE(15)
  }
}

void build_lookup_table() {
  decrement_if_positive_lookup[0] = 0;
  for (int i = 1; i < 256; i++) {
    decrement_if_positive_lookup[i] = u8(i) - 2;
  }
}

void zero_data(u8 (&scanline_data)[GPU_WIDTH]) {
  for (int x = 0; x < GPU_WIDTH; x++) {
    scanline_data[x] = 0;
  }
}

void alternating_data(u8 (&scanline_data)[GPU_WIDTH]) {
  // Even columns will end, odd columns will cycle forever.
  for (int x = 0; x < GPU_WIDTH; x++) {
    scanline_data[x] = (x & 1) ? x : 0;
  }
}

void all_data(u8 (&scanline_data)[GPU_WIDTH]) {
  // All columns cycle.
  for (int x = 0; x < GPU_WIDTH; x++) {
    scanline_data[x] = x + (x & 1) + 1;
  }
}

} // namespace


BENCHMARK_FUNC(decrement) {
  const int num_iterations = 1000;
  ALIGNAS(4) u8 scanline_data[GPU_WIDTH];
  build_lookup_table();

  zero_data(scanline_data);
  BENCHMARK_RUN(basic_impl_zero, num_iterations) {
    basic_impl(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_8_zero, num_iterations) {
    unrolled_impl_8(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_16_zero, num_iterations) {
    unrolled_impl_16(scanline_data);
  }

  alternating_data(scanline_data);
  BENCHMARK_RUN(basic_impl_alternating, num_iterations) {
    basic_impl(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_8_alternating, num_iterations) {
    unrolled_impl_8(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_16_alternating, num_iterations) {
    unrolled_impl_16(scanline_data);
  }

  all_data(scanline_data);
  BENCHMARK_RUN(basic_impl_all, num_iterations) {
    basic_impl(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_8_all, num_iterations) {
    unrolled_impl_8(scanline_data);
  }
  BENCHMARK_RUN(unrolled_impl_16_all, num_iterations) {
    unrolled_impl_16(scanline_data);
  }
}
