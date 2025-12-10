#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "funcs.h"
#include "keyboard.h"
#include "macros.h"
#include "sound.h"
#include "types.h"

#define NUM_LEVELS 4

// Common voice handles for shared state.
#define VOICE_HANDLE_MENU_BGM 0
#define VOICE_HANDLE_MENU_CLICK 1

namespace game {

#define GAME_DATA_PATH(path) "aaa/data/" path

extern bool g_had_error;
extern bool g_sound_enabled;
extern u8 g_level_selected;
extern u8 g_unlocked_levels;
extern bool g_invincible;

void toggle_audio();
void load_menu_audio();

#define play_click_sound() soundsystem::play(VOICE_HANDLE_MENU_CLICK)

} // game

#endif
