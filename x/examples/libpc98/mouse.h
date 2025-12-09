#ifndef MOUSE_H
#define MOUSE_H

#include "funcs.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>

#ifndef WEB_BUILD
#include <pc.h>
#endif

namespace mouse {

// Running dosbox-x in windows sandbox doesn't lock the cursor
// so you get broken mouse movement.
// sensitivity = 1,1 needs adding to the [sdl] section too.
#define MOUSE_BROKEN_CURSOR_LOCK 0

// Read the status of the mouse (read):
//   7:7 - left button up
//   6:6 - middle button up
//   5:5 - right button up
//   4:4 - ???
//   3:0 - mouse delta
#define PORT_MOUSE_STATUS 0x7FD9
#define BIT_MOUSE_LEFT (1 << 7)
#define BIT_MOUSE_MIDDLE (1 << 6)
#define BIT_MOUSE_RIGHT (1 << 5)
// Mouse control (write):
//   7:7 - clear delta after read
//   6:5 - which delta to read/write
#define PORT_MOUSE_CONTROL 0x7FDD
#define BIT_MOUSE_RESET 0x80 // 1xxxxxxx
#define BIT_MOUSE_X_LO 0x00 // x00xxxxx
#define BIT_MOUSE_X_HI 0x20 // x01xxxxx
#define BIT_MOUSE_Y_LO 0x40 // x10xxxxx
#define BIT_MOUSE_Y_HI 0x60 // x11xxxxx

typedef u8 ButtonsState;
struct Button {
    enum E {
        Left = BIT_MOUSE_LEFT,
        Middle = BIT_MOUSE_MIDDLE,
        Right = BIT_MOUSE_RIGHT,
    };
};

#ifdef WEB_BUILD

void init();
void read_delta(i8 &dx, i8 &dy);
ButtonsState read_buttons();

#else // WEB_BUILD

static FORCEINLINE void init() {
  // Reset the mouse tracking.
  outportb(PORT_MOUSE_CONTROL, BIT_MOUSE_RESET);
  outportb(PORT_MOUSE_CONTROL, 0x00);
}

static FORCEINLINE void read_delta(i8 &dx, i8 &dy) {
#if MOUSE_BROKEN_CURSOR_LOCK
  // Don't reset if we're using the hack. We'll do the delta manually.
  #undef BIT_MOUSE_RESET
  #define BIT_MOUSE_RESET 0
#endif // MOUSE_BROKEN_CURSOR_LOCK

  // Read off the deltas.
  outportb(PORT_MOUSE_CONTROL, BIT_MOUSE_RESET | BIT_MOUSE_X_LO);
  u8 xlo = static_cast<u8>(inportb(PORT_MOUSE_STATUS)) & 0xF;
  outportb(PORT_MOUSE_CONTROL, BIT_MOUSE_RESET | BIT_MOUSE_X_HI);
  u8 xhi = static_cast<u8>(inportb(PORT_MOUSE_STATUS)) & 0xF;
  outportb(PORT_MOUSE_CONTROL, BIT_MOUSE_RESET | BIT_MOUSE_Y_LO);
  u8 ylo = static_cast<u8>(inportb(PORT_MOUSE_STATUS)) & 0xF;
  outportb(PORT_MOUSE_CONTROL, BIT_MOUSE_RESET | BIT_MOUSE_Y_HI);
  u8 yhi = static_cast<u8>(inportb(PORT_MOUSE_STATUS)) & 0xF;

  // Start accumulating movement deltas again.
  outportb(PORT_MOUSE_CONTROL, 0x00);

  // Build output for caller.
  dx = static_cast<i8>(xhi << 4) | xlo;
  dy = static_cast<i8>(yhi << 4) | ylo;

  // Add hack if we need it.
#if MOUSE_BROKEN_CURSOR_LOCK
  static i8 dx_, dy_;
  i8 t;
  t = dx; dx -= dx_; dx_ = t;
  t = dy; dy -= dy_; dy_ = t;
#endif // MOUSE_BROKEN_CURSOR_LOCK
}

static FORCEINLINE ButtonsState read_buttons() {
    return ~inportb(PORT_MOUSE_STATUS);
}

#endif // WEB_BUILD

static FORCEINLINE bool is_button_pressed(ButtonsState state, Button::E button) {
    return state & static_cast<u8>(button);
}

} // namespace mouse

#endif
