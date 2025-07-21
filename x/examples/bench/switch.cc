//
// Tests for beating GCC at switch implementations
// (because it doesn't know about the domain)
//

#include "bench.h"
#include "gpuscrn.h"

#include <cstdio>

namespace {

FORCEINLINE int bank_split_switch(int line) {
  switch (line) {
    case 51: return 128;
    case 102: return 256;
    case 153: return 384;
    case 204: return 512;
    case 255: return 0;
    case 307: return 128;
    case 358: return 256;
    default: return 0;
  }
}

FORCEINLINE u16 bank_split_switch_u16(u16 line) {
  switch (line) {
    case 51: return 128;
    case 102: return 256;
    case 153: return 384;
    case 204: return 512;
    case 255: return 0;
    case 307: return 128;
    case 358: return 256;
    default: return 0;
  }
}

FORCEINLINE int bank_split_less_branches(unsigned line) {
  unsigned l = line - (line >> 8); // if line > 255, line -= 1
  if (l % 51) return 0;
  unsigned q = l / 51; // TODO: gcc doesn't reuse the result of the div
  return ((q > 4) ? q - 5 : q) << 7; // q * 128
}

} // namespace


BENCHMARK_FUNC(switch_case) {
  const int num_iterations = 100000;

  volatile int line;
  volatile int split;

  for (line = 0; line < GPU_HEIGHT; line++) {
    if (bank_split_switch(line) != bank_split_switch_u16(line)) {
      printf("ERROR: bug in u16 version: %i\n", line);
      return;
    }
    if (bank_split_switch(line) != bank_split_less_branches(line)) {
      printf("ERROR: bug in branchless version: %i\n", line);
      return;
    }
  }


#define bank_split_switch_test(l) \
  BENCHMARK_RUN(bank_split_switch_##l, num_iterations) { \
    line = l; \
    split = bank_split_switch(line); \
  }

  bank_split_switch_test(0);
  bank_split_switch_test(1);
  bank_split_switch_test(51);
  bank_split_switch_test(255);
  bank_split_switch_test(256);
  bank_split_switch_test(307);


#define bank_split_switch_u16_test(l) \
  BENCHMARK_RUN(bank_split_switch_u16_##l, num_iterations) { \
    line = l; \
    split = bank_split_switch_u16(line); \
  }

  bank_split_switch_u16_test(0);
  bank_split_switch_u16_test(1);
  bank_split_switch_u16_test(51);
  bank_split_switch_u16_test(255);
  bank_split_switch_u16_test(256);
  bank_split_switch_u16_test(307);


#define bank_split_less_branches_test(l) \
  BENCHMARK_RUN(bank_split_less_branches_##l, num_iterations) { \
    line = l; \
    split = bank_split_less_branches(line); \
  }

  bank_split_less_branches_test(0);
  bank_split_less_branches_test(1);
  bank_split_less_branches_test(51);
  bank_split_less_branches_test(255);
  bank_split_less_branches_test(256);
  bank_split_less_branches_test(307);
}
