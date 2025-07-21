#include "bench.h"
#include "logs.h"

#include <cstdio>

namespace benchmarking {

namespace {

Benchmark *s_active;

} // namespace

void run_all() {
  printf("Report will be written to bench.txt\n");
  logging::init("bench.txt");

  Benchmark *info = s_active;
  while (info != NULL) {
    printf("Running %s:\n", info->name);
    info->func(*info);
    info = info->next;
break;
  }

  printf("Finished\n");
}

Timer::Timer(const char *n, int i) : name(n), max(i), counter(0) {
  start = Funcs98::ticks();
  printf("  - %s\n", n);
}

Timer::~Timer() {
  uclock_t stop = Funcs98::ticks();
  uclock_t ticks = stop - start;
  logging::print(logging::Level::Info, "  %s: %lld ticks/iter (%lld total)", name, ticks / max, ticks);
}

int registr(Benchmark *info) {
  // Add it.
  info->next = s_active;
  s_active = info;

  // Unused.
  return 0;
}

} // namespace benchmarking
