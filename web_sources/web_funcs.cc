#include "funcs.h"
#include "keyboard.h"

#include "web_common.h"

#include <chrono>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

namespace {

const bool *s_sdl_keys;

} // namespace

void ScreenPutString_98(const char *text, unsigned colour, int x, int y) {
  // TODO
  (void)text;
  (void)colour;
  (void)x;
  (void)y;
}

void ScreenPutChar_98(char text, unsigned colour, int x, int y) {
  // TODO
  (void)text;
  (void)colour;
  (void)x;
  (void)y;
}

//

void Funcs64::clear_screen() {
  // TODO
}

void Funcs64::pc_beep(int freq) {
  // TODO
  (void)freq;
}

bool Funcs64::kb_hit() {
  // Refresh the keyboard state.
  SDL_PumpEvents();
  s_sdl_keys = SDL_GetKeyboardState(nullptr);
  return getch_98();
}

uclock_t Funcs64::ticks() {
  auto now = std::chrono::system_clock::now();
  static auto begin = now;
  return std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
}

//

int getch_98() {
  if (!s_sdl_keys) {
    return 0;
  }

  // This is called right after kb_hit() so just read it off.
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_Q]) return 'q';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_W]) return 'w';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_E]) return 'e';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_T]) return 't';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_A]) return 'a';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_S]) return 's';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_D]) return 'd';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_RETURN]) return '\r';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_KP_ENTER]) return '\r';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_SPACE]) return ' ';
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_DOWN]) return 10;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_UP]) return 11;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_LEFT]) return 8;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_RIGHT]) return 12;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_ESCAPE]) return 27;
  return 0;
}

//

u32 read_keyboard_state() {
  // Refresh current state.
  Funcs64::kb_hit();

  u32 bits = 0;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_Q]) bits |= KB_STATE_Q;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_W]) bits |= KB_STATE_W;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_E]) bits |= KB_STATE_E;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_T]) bits |= KB_STATE_T;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_A]) bits |= KB_STATE_A;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_S]) bits |= KB_STATE_S;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_D]) bits |= KB_STATE_D;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_RETURN]) bits |= KB_STATE_ENTER;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_KP_ENTER]) bits |= KB_STATE_ENTER;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_SPACE]) bits |= KB_STATE_SPACE;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_DOWN]) bits |= KB_STATE_DOWN;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_UP]) bits |= KB_STATE_UP;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_LEFT]) bits |= KB_STATE_LEFT;
  if (s_sdl_keys[SDL_Scancode::SDL_SCANCODE_RIGHT]) bits |= KB_STATE_RIGHT;
  return bits;
}
