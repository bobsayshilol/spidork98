#ifndef FUNCS_H
#define FUNCS_H

#include "macros.h"

#ifndef WEB_BUILD

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

#else

#include <cstdlib>
#include <cstring>
#include <time.h>
#include <unistd.h>

#define ISPC98(x) 1

#define ScreenCols_98() 80
#define ScreenRows_98() 24

void ScreenPutString_98(const char *text, unsigned colour, int x, int y);
void ScreenPutChar_98(char text, unsigned colour, int x, int y);

using uclock_t = clock_t;
STATIC_ASSERT(sizeof(uclock_t) == sizeof(size_t));

struct Funcs64 {
  static const char *name() { return "Web"; }
  static void clear_screen();
#ifdef __EMSCRIPTEN__
  static void delay_ms(unsigned) { }
#else
  static void delay_ms(unsigned msec) { usleep(msec * 1000); }
#endif
  static void pc_beep(int freq);
  static bool kb_hit();
  static uclock_t ticks();
  static uclock_t ticks_per_sec() { return 1'000'000; }
};

using FuncsAT = Funcs64;
using Funcs98 = Funcs64;

#endif

// The colours in conio.h are lies.
#define COLOUR_BLUE 0x1
#define COLOUR_GREEN 0x2
#define COLOUR_RED 0x4
#define COLOUR_CYAN (COLOUR_BLUE | COLOUR_GREEN)
#define COLOUR_MAGENTA (COLOUR_BLUE | COLOUR_RED)
#define COLOUR_YELLOW (COLOUR_GREEN | COLOUR_RED)
#define COLOUR_WHITE (COLOUR_BLUE | COLOUR_GREEN | COLOUR_RED)
#define COLOUR_UNDERLINE 0x8

#endif
