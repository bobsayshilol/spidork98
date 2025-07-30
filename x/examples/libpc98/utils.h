#ifndef UTILS_H
#define UTILS_H

// libstdc++ is very broken due to 8.3 filenames, so this is a polyfill.
//#include <utility>

namespace utils {

template <typename T>
FORCEINLINE static void swap(T & l,T & r) {
  T t = l;
  l = r;
  r = t;
}

template <typename T>
FORCEINLINE static T min(T l,T r) {
  return l < r ? l : r;
}

template <typename T>
FORCEINLINE static T max(T l,T r) {
  return l > r ? l : r;
}

template <typename T>
FORCEINLINE static T clamp(T x, T lo, T hi) {
  return x > lo ? (x < hi ? x : hi) : lo;
}

template <typename T>
static void randomise(T * p, int n) {
  for (int i = 0; i < n; i++) {
    const int j = rand() % (i + 1);
    swap(p[i], p[j]);
  }
}

} // namespace utils

#endif
