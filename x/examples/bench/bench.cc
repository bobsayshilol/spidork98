#include "bench.h"

#include <cstdio>

namespace benchmarking {

namespace {

Benchmark *s_active;

} // namespace

void run_all() {
  Benchmark *info = s_active;
  while (info != NULL) {
    printf("Running %s:\n", info->name);
    info->func(*info);
    info = info->next;
  }
}

Timer::Timer(const char *n, int i) : name(n), max(i), counter(0) {
  start = Funcs98::ticks();
}

Timer::~Timer() {
  uclock_t stop = Funcs98::ticks();
  uclock_t ticks = stop - start;
  printf("  %s: %lld ticks/iter (%lld total)\n", name, ticks / max, ticks);
}

int registr(Benchmark *info) {
  // Add it.
  info->next = s_active;
  s_active = info;

  // Unused.
  return 0;
}

} // namespace benchmarking
