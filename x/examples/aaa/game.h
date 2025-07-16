#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "funcs.h"
#include "macros.h"
#include "types.h"

#include <conio.h>

namespace game {

#define GAME_DATA_PATH(path) "aaa/" path

extern bool g_had_error;

static FORCEINLINE void flush_kb_buffer() {
  while (kbhit_98()) getch();
}

} // game

#endif
