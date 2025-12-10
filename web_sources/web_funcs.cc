#include "funcs.h"
#include "keyboard.h"
#include "mouse.h"

#include "web_common.h"

#include <chrono>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>

namespace {

const bool *s_sdl_keys;
bool s_last_sdl_keys[SDL_SCANCODE_COUNT];

} // namespace

void ScreenPutString_98(const char *text, unsigned colour, int x, int y) {
  char ch;
  while ((ch = *text) != '\0') {
    ScreenPutChar_98(ch, colour, x, y);
    text++;
    x++;
  }
}

void ScreenPutChar_98(char text, unsigned colour, int x, int y) {
  web::print_text_layer(text, colour, x, y);
}

//

void Funcs64::clear_screen() {
  web::clear_text_layer();
}

void Funcs64::pc_beep(int freq) {
  // TODO
  (void)freq;
}

bool Funcs64::kb_hit() {
  // Refresh the keyboard state.
  SDL_PumpEvents();
  s_sdl_keys = SDL_GetKeyboardState(nullptr);

  for (int i = 0; i < SDL_Scancode::SDL_SCANCODE_COUNT; i++) {
    if (s_sdl_keys[i] != s_last_sdl_keys[i]) {
      return true;
    }
  }
  return false;
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

#define IF_WAS_PRESSED(scancode, ch) do { \
    const bool is_pressed = s_sdl_keys[SDL_Scancode::SDL_SCANCODE_##scancode]; \
    bool & was_pressed = s_last_sdl_keys[SDL_Scancode::SDL_SCANCODE_##scancode]; \
    const bool did_press = is_pressed && !was_pressed; \
    was_pressed = is_pressed; \
    if (did_press) { \
      return ch; \
    } \
  } while (false)

  // This is called right after kb_hit() so just read it off.
  IF_WAS_PRESSED(Q, 'q');
  IF_WAS_PRESSED(W, 'w');
  IF_WAS_PRESSED(E, 'e');
  IF_WAS_PRESSED(R, 'r');
  IF_WAS_PRESSED(T, 't');
  IF_WAS_PRESSED(Y, 'y');
  IF_WAS_PRESSED(U, 'u');
  IF_WAS_PRESSED(I, 'i');
  IF_WAS_PRESSED(O, 'o');
  IF_WAS_PRESSED(P, 'p');
  IF_WAS_PRESSED(A, 'a');
  IF_WAS_PRESSED(S, 's');
  IF_WAS_PRESSED(D, 'd');
  IF_WAS_PRESSED(U, 'u');
  IF_WAS_PRESSED(I, 'i');
  IF_WAS_PRESSED(RETURN, '\r');
  IF_WAS_PRESSED(KP_ENTER, '\r');
  IF_WAS_PRESSED(SPACE, ' ');
  IF_WAS_PRESSED(DOWN, 10);
  IF_WAS_PRESSED(UP, 11);
  IF_WAS_PRESSED(LEFT, 8);
  IF_WAS_PRESSED(RIGHT, 12);
  IF_WAS_PRESSED(ESCAPE, 27);

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

//

void mouse::init() {
  SDL_GetRelativeMouseState(nullptr, nullptr);
}

void mouse::read_delta(i8 &dx, i8 &dy) {
  float fdx = 0, fdy = 0;
  SDL_GetRelativeMouseState(&fdx, &fdy);
  dx = fdx;
  dy = fdy;
}

mouse::ButtonsState mouse::read_buttons() {
  ButtonsState state = 0;
  const SDL_MouseButtonFlags flags = SDL_GetRelativeMouseState(nullptr, nullptr);
  if (flags & SDL_BUTTON_LMASK) state |= Button::Left;
  if (flags & SDL_BUTTON_MMASK) state |= Button::Middle;
  if (flags & SDL_BUTTON_RMASK) state |= Button::Right;
  return state;
}
