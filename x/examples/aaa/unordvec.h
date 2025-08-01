#ifndef UNORDERED_VECTOR_H
#define UNORDERED_VECTOR_H

#include "logs.h"
#include "types.h"

#include <cstdlib>

namespace game {

#define DEBUG_CHECK_VECTOR 0

template <typename T, u16 N>
struct StaticUnorderedVector {
  StaticUnorderedVector() : count(0) {
#if DEBUG_CHECK_VECTOR
  m_max_size = 0;
#endif
  }

#if DEBUG_CHECK_VECTOR
  u16 m_max_size;
  void log(const char *msg) {
    logging::print(logging::Level::Info, "%s: max_size: %u", msg, m_max_size);
    m_max_size = 0;
  }
#endif

  FORCEINLINE T &add() {
#if DEBUG_CHECK_VECTOR
    if (count >= N) {
      logging::print(logging::Level::Error, "ERROR: capacity exceeded");
      exit(2);
    }
#endif
    count++;
#if DEBUG_CHECK_VECTOR
    if (count > m_max_size) m_max_size = count;
#endif
    return raw[count - 1];
  }

  FORCEINLINE T *try_add() {
    if (count >= N) {
      return 0;
    }
    count++;
#if DEBUG_CHECK_VECTOR
    if (count > m_max_size) m_max_size = count;
#endif
    return &raw[count - 1];
  }

  FORCEINLINE void erase_at(u16 idx) {
#if DEBUG_CHECK_VECTOR
    if (idx >= count) {
      logging::print(logging::Level::Error, "ERROR: trying to erase bad element");
      exit(2);
    }
#endif
    count--;
    //memmove(raw + idx, raw + idx + 1, (count - idx) * sizeof(T));
    raw[idx] = raw[count];
  }

  void clear() {
    count = 0;
  }

  u16 count;
  T raw[N];
};

} // namespace game

#endif
