#pragma once

extern "C" {
#include <libc/pc9800.h>
}

namespace {

struct FuncsAT {
  static inline const char *name() { return "PC-AT Compatible"; }
  static inline void ScreenClear() { ScreenClear_at(); }
  static inline void delay(unsigned msec) { delay_at(msec); }
  static inline void sound(int freq) { sound_at(freq); }
};

struct Funcs98 {
  static inline const char *name() { return "PC-98"; }
  static inline void ScreenClear() { ScreenClear_98(); }
  static inline void delay(unsigned msec) { delay_98(msec); }
  static inline void sound(int freq) { sound_98(freq); }
};

} // namespace
