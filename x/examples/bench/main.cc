#include "bench.h"

#include <cstdlib>
#include <cstdio>

int main() {
  printf("Running all benchmarks\n");
  benchmarking::run_all();
  return EXIT_SUCCESS;
}
