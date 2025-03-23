#ifndef FUNCS_H
#define FUNCS_H

#include "macros.h"

extern "C" {
#include <libc/pc9800.h>
}

namespace {

struct FuncsAT {
  static FORCEINLINE const char *name() { return "PC-AT Compatible"; }
  static FORCEINLINE void clear_screen() { clrscr_at(); }
  static FORCEINLINE void delay_ms(unsigned msec) { delay_at(msec); }
  static FORCEINLINE void pc_beep(int freq) { sound_at(freq); }
  static FORCEINLINE bool kb_hit() { return kbhit_at(); }
  static FORCEINLINE uclock_t ticks() { return uclock_at(); }
  static FORCEINLINE uclock_t ticks_per_sec() { return UCLOCKS_PER_SEC; }
};

struct Funcs98 {
  static FORCEINLINE const char *name() { return "PC-98"; }
  static FORCEINLINE void clear_screen() { clrscr_98(); }
  static FORCEINLINE void delay_ms(unsigned msec) { delay_98(msec); }
  static FORCEINLINE void pc_beep(int freq) { sound_98(freq); }
  static FORCEINLINE bool kb_hit() { return kbhit_98(); }
  static FORCEINLINE uclock_t ticks() { return uclock_98(); }
  static FORCEINLINE uclock_t ticks_per_sec() { return UCLOCKS_PER_SEC; }
};

} // namespace

#endif
