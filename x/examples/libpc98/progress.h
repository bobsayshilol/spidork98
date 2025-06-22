#ifndef PROGRESS_H
#define PROGRESS_H

#include "types.h"

#include <cstdio>
#include <cstdlib>

namespace {

// Progress tracker.
// Prints out the percentage every |every| items processed.
struct Progress {
  Progress(u32 total, u32 every)
    : m_current(0)
    , m_max(total)
    , m_every(every)
    , m_next(0)
  {
  }

  void increment(u32 items = 1) {
    m_current += items;
    while (m_current > m_next) {
      const u32 p = m_next * 100 / m_max;
      printf("%i%%\n", p);
      m_next += m_every;
    }
    if (m_current == m_max) {
      printf("100%%\n");
    }
  }

private:
  u32 m_current;
  u32 m_max;
  u32 m_every;
  u32 m_next;
};

} // namespace


#endif
