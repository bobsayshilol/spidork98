#include "maths.h"

#include <cstdio>
#include <cstdlib>

namespace {

bool check_fpu() {
  volatile float a = 1.f;
  volatile float b = 2.f;
  if (a * b != b) return false;
  if (a + a != b) return false;
  return a > 0;
}

} // namespace

int main() {
  if (!check_fpu()) {
    printf("Tests can't pass without an FPU\n");
    return EXIT_FAILURE;
  }

  printf("Running tests\n");
  for (int i = 0; i < 256; i++) {
    const signed char ms = maths::sin(i);
    const signed char rs = static_cast<int>(127 * sin(2 * 3.14159f * i / 256));
    if (abs(ms - rs) > 1) {
      printf("Failed: i=%i ms=%i rs=%i\n", i, ms, rs);
      return EXIT_FAILURE;
    }

    const signed char mc = maths::cos(i);
    const signed char rc = static_cast<int>(127 * cos(2 * 3.14159f * i / 256));
    if (abs(mc - rc) > 1) {
      printf("Failed: i=%i mc=%i rc=%i\n", i, mc, rc);
      return EXIT_FAILURE;
    }
  }

  printf("All tests passed\n");
  return EXIT_SUCCESS;
}
