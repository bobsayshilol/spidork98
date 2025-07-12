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

} // namespace utils

#endif
