#ifndef FUNCS_H
#define FUNCS_H

extern "C" {
#include <libc/pc9800.h>
}

namespace {

struct FuncsAT {
  static __inline__ const char *name() { return "PC-AT Compatible"; }
  static __inline__ void clear_screen() { clrscr_at(); }
  static __inline__ void delay_ms(unsigned msec) { delay_at(msec); }
  static __inline__ void pc_beep(int freq) { sound_at(freq); }
  static __inline__ bool kb_hit() { return kbhit_at(); }
  static __inline__ clock_t now() { return clock_at(); }
};

struct Funcs98 {
  static __inline__ const char *name() { return "PC-98"; }
  static __inline__ void clear_screen() { clrscr_98(); }
  static __inline__ void delay_ms(unsigned msec) { delay_98(msec); }
  static __inline__ void pc_beep(int freq) { sound_98(freq); }
  static __inline__ bool kb_hit() { return kbhit_98(); }
  static __inline__ clock_t now() { return clock_98(); }
};

} // namespace

#endif
