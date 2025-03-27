//
// Benchmarking helpers
//
// Usage:
//
//   BENCHMARK_FUNC(a_benchmark) {
//     init();
//
//     BENCHMARK_RUN(loop1, 1000) {
//       loop1();
//     }
//
//     BENCHMARK_RUN(loop2, 1000) {
//       loop2();
//     }
//   }
//

#include "funcs.h"
#include "macros.h"

#define BENCHMARK_FUNC(name) \
  static void _benchmark_##name(benchmarking::Benchmark &_benchmark_info); \
  static benchmarking::Benchmark _benchmark_info_##name = { #name, _benchmark_##name, NULL }; \
  static UNUSED int _benchmark_register_##name = benchmarking::registr(&_benchmark_info_##name); \
  void _benchmark_##name(benchmarking::Benchmark &_benchmark_info)

#define BENCHMARK_RUN(name, iters) \
  for (benchmarking::Timer _timer(#name, iters); _timer.counter < _timer.max; _timer.counter++)

namespace benchmarking {

// Run all the benchmarks.
void run_all();

//
// Implementation details.
//

struct Benchmark {
  const char *name;
  void (*func)(Benchmark&);
  Benchmark *next;
};

struct Timer {
  const char *name;
  int max;
  int counter;
  uclock_t start;
  Timer(const char *, int);
  ~Timer();
};

int registr(Benchmark *info);

} // benchmarking
