#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "funcs.h"
#include "macros.h"
#include "sound.h"
#include "types.h"

#include <conio.h>

// The colours in conio.h are lies.
#define COLOUR_BLUE 0x1
#define COLOUR_GREEN 0x2
#define COLOUR_RED 0x4
#define COLOUR_CYAN (COLOUR_BLUE | COLOUR_GREEN)
#define COLOUR_MAGENTA (COLOUR_BLUE | COLOUR_RED)
#define COLOUR_YELLOW (COLOUR_GREEN | COLOUR_RED)
#define COLOUR_WHITE (COLOUR_BLUE | COLOUR_GREEN | COLOUR_RED)
#define COLOUR_UNDERLINE 0x8

// Extra input keys.
#define KEY_UP 11
#define KEY_DOWN 10
#define KEY_LEFT 8
#define KEY_RIGHT 12
#define KEY_ENTER '\r'
#define KEY_SPACE ' '
#define KEY_ESCAPE 27

#define NUM_LEVELS 4

// Common voice handles for shared state.
#define VOICE_HANDLE_MENU_BGM 0
#define VOICE_HANDLE_MENU_CLICK 1

namespace game {

#define GAME_DATA_PATH(path) "aaa/" path

extern bool g_had_error;
extern bool g_sound_enabled;
extern u8 g_level_selected;

static FORCEINLINE void flush_kb_buffer() {
  while (kbhit_98()) getch();
}

void toggle_audio();
void load_menu_audio();

#define play_click_sound() soundsystem::play(VOICE_HANDLE_MENU_CLICK)

} // game

#endif
